//	MIT License
//	
//	Copyright(c) 2017 Thomas Monkman
//	
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files(the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions :
//	
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.

#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <cstdio>
#include <fstream>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <Pathcch.h>
#include <shlwapi.h>
#endif // WIN32

#if __unix__
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#endif // __unix__

#ifdef __linux__
#include <linux/limits.h>
#endif

#if defined(__APPLE__) || defined(__MACH__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#define FILEWATCH_PLATFORM_MAC 1
#endif

#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <system_error>
#include <string>
#include <algorithm>
#include <type_traits>
#include <future>
#include <regex>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <cassert>
#include <cstdlib>
#include <iostream>

#ifdef FILEWATCH_PLATFORM_MAC
extern "C" int __getdirentries64(int, char *, int, long *);
#endif // FILEWATCH_PLATFORM_MAC

namespace filewatch {
	enum class Event {
		added,
		removed,
		modified,
		renamed_old,
		renamed_new
	};
      
      template<typename StringType>
      struct IsWChar {
            static constexpr bool value = false;
      };

      template<> 
      struct IsWChar<wchar_t> {
            static constexpr bool value = true;
      };

      template<typename Fn, typename... Args>
      struct Invokable {
            static Fn make() {
                  return (Fn*)0;
            }

            template<typename T>
            static T defaultValue() {
                  return *(T*)0;
            }

            static void call(int) {
                  make()(defaultValue<Args...>());
            }

            static int call(long value);

            static constexpr bool value = std::is_same<decltype(call(0)), int>::value;
      };

#define _FILEWATCH_TO_STRING(x) #x
#define FILEWATCH_TO_STRING(x) _FILEWATCH_TO_STRING(x)

      [[maybe_unused]] static const char* event_to_string(Event event) {
            switch (event) {
            case Event::added:
                  return FILEWATCH_TO_STRING(Event::added);
            case Event::removed:
                  return FILEWATCH_TO_STRING(Event::removed);
            case Event::modified:
                  return FILEWATCH_TO_STRING(Event::modified);
            case Event::renamed_old:
                  return FILEWATCH_TO_STRING(Event:renamed_old);
            case Event::renamed_new:
                  return FILEWATCH_TO_STRING(Event::renamed_new);
            }
            assert(false);
      }

      template<typename StringType>
      static typename std::enable_if<std::is_same<typename StringType::value_type, wchar_t>::value, bool>::type 
      isParentOrSelfDirectory(const StringType& path) {
            return path == L"." || path == L"..";
      }

      template<typename StringType>
      static typename std::enable_if<std::is_same<typename StringType::value_type, char>::value, bool>::type 
      isParentOrSelfDirectory(const StringType& path) {
            return path == "." || path == "..";
      }

	/**
	* \class FileWatch
	*
	* \brief Watches a folder or file, and will notify of changes via function callback.
	*
	* \author Thomas Monkman
	*
	*/
	template<class StringType>
	class FileWatch
	{
		typedef typename StringType::value_type C;
		typedef std::basic_string<C, std::char_traits<C>> UnderpinningString;
		typedef std::basic_regex<C, std::regex_traits<C>> UnderpinningRegex;

	public:

		FileWatch(StringType path, UnderpinningRegex pattern, std::function<void(const StringType& file, const Event event_type)> callback) :
			_path(absolute_path_of(path)),
			_pattern(pattern),
			_callback(callback),
                  _directory(get_directory(path))
		{
			init();
		}

		FileWatch(StringType path, std::function<void(const StringType& file, const Event event_type)> callback) :
			FileWatch<StringType>(path, UnderpinningRegex(_regex_all), callback) {}

		~FileWatch() {
			destroy();
		}

		FileWatch(const FileWatch<StringType>& other) : FileWatch<StringType>(other._path, other._callback) {}

		FileWatch<StringType>& operator=(const FileWatch<StringType>& other) 
		{
			if (this == &other) { return *this; }

			destroy();
			_path = other._path;
			_callback = other._callback;
			_directory = get_directory(other._path);
			init();
			return *this;
		}

		// Const memeber varibles don't let me implent moves nicely, if moves are really wanted std::unique_ptr should be used and move that.
		FileWatch<StringType>(FileWatch<StringType>&&) = delete;
		FileWatch<StringType>& operator=(FileWatch<StringType>&&) & = delete;

	private:
		static constexpr C _regex_all[] = { '.', '*', '\0' };
		static constexpr C _this_directory[] = { '.', '/', '\0' };

		struct PathParts
		{
			PathParts(StringType directory, StringType filename) : directory(directory), filename(filename) {}
			StringType directory;
			StringType filename;
		};
		const StringType _path;

		UnderpinningRegex _pattern;

		static constexpr std::size_t _buffer_size = { 1024 * 256 };

		// only used if watch a single file
		StringType _filename;

		std::function<void(const StringType& file, const Event event_type)> _callback;

		std::thread _watch_thread;

		std::condition_variable _cv;
		std::mutex _callback_mutex;
		std::vector<std::pair<StringType, Event>> _callback_information;
		std::thread _callback_thread;

		std::promise<void> _running;
		std::atomic<bool> _destory = { false };
		bool _watching_single_file = { false };

#pragma mark "Platform specific data"
#ifdef _WIN32
		HANDLE _directory = { nullptr };
		HANDLE _close_event = { nullptr };

		const DWORD _listen_filters =
			FILE_NOTIFY_CHANGE_SECURITY |
			FILE_NOTIFY_CHANGE_CREATION |
			FILE_NOTIFY_CHANGE_LAST_ACCESS |
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_FILE_NAME;

		const std::unordered_map<DWORD, Event> _event_type_mapping = {
			{ FILE_ACTION_ADDED, Event::added },
			{ FILE_ACTION_REMOVED, Event::removed },
			{ FILE_ACTION_MODIFIED, Event::modified },
			{ FILE_ACTION_RENAMED_OLD_NAME, Event::renamed_old },
			{ FILE_ACTION_RENAMED_NEW_NAME, Event::renamed_new }
		};
#endif // WIN32

#if __unix__
		struct FolderInfo {
			int folder;
			int watch;
		};

		FolderInfo  _directory;

		const std::uint32_t _listen_filters = IN_MODIFY | IN_CREATE | IN_DELETE;

		const static std::size_t event_size = (sizeof(struct inotify_event));
#endif // __unix__

#if FILEWATCH_PLATFORM_MAC
            struct FileState 
            {
                  int fd;
                  uint32_t nlink;
                  time_t last_modification;

                  FileState(int fd, uint32_t nlink, time_t lt) 
                        : fd(fd), nlink(nlink), 
                        last_modification(lt)
                  {

                  }
                  FileState(const FileState&) = delete;
                  FileState& operator=(const FileState&) = delete;
                  FileState(FileState&& other) : fd(other.fd), nlink(other.nlink), last_modification(other.last_modification)
                  {
                        other.fd = -1;
                  }

                  FileState invalidate_and_clone() {
                        int fd = this->fd;

                        this->fd = -1;
                        return FileState {fd, nlink, last_modification};
                  }

                  ~FileState() 
                  {
                        if (fd != -1) {
                              close(fd);
                        }
                  }
            };
            std::unordered_map<StringType, FileState> _directory_snapshot{};
            bool _previous_event_is_rename = false;
            CFRunLoopRef _run_loop = nullptr;
            int _file_fd = -1;
            struct timespec _last_modification_time = {};
            FSEventStreamRef _directory;
            // fd for single file
#endif // FILEWATCH_PLATFORM_MAC

		void init() 
		{
#ifdef _WIN32
			_close_event = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (!_close_event) {
				throw std::system_error(GetLastError(), std::system_category());
			}
#endif // WIN32

			_callback_thread = std::thread([this]() {
				try {
					callback_thread();
				} catch (...) {
					try {
						_running.set_exception(std::current_exception());
					}
					catch (...) {} // set_exception() may throw too
				}
			});

			_watch_thread = std::thread([this]() { 
				try {
					monitor_directory();
				} catch (...) {
					try {
						_running.set_exception(std::current_exception());
					}
					catch (...) {} // set_exception() may throw too
				}
			});

			std::future<void> future = _running.get_future();
			future.get(); //block until the monitor_directory is up and running
		}

		void destroy()
		{
			_destory = true;
			_running = std::promise<void>();

#ifdef _WIN32
			SetEvent(_close_event);
#elif __unix__
			inotify_rm_watch(_directory.folder, _directory.watch);
#elif FILEWATCH_PLATFORM_MAC
                  if (_run_loop) {
                        CFRunLoopStop(_run_loop);
                  }
#endif // __unix__

			_cv.notify_all();
			_watch_thread.join();
			_callback_thread.join();

#ifdef _WIN32
			CloseHandle(_directory);
#elif __unix__
			close(_directory.folder);
#elif FILEWATCH_PLATFORM_MAC
                  FSEventStreamStop(_directory);
                  FSEventStreamInvalidate(_directory);
                  FSEventStreamRelease(_directory);
                  _directory = nullptr;
#endif // FILEWATCH_PLATFORM_MAC
		}

		const PathParts split_directory_and_file(const StringType& path) const 
		{
			const auto predict = [](C character) {
#ifdef _WIN32
				return character == C('\\') || character == C('/');
#elif __unix__ || FILEWATCH_PLATFORM_MAC
				return character == C('/');
#endif // __unix__
			};

			UnderpinningString path_string = path;
			const auto pivot = std::find_if(path_string.rbegin(), path_string.rend(), predict).base();
			//if the path is something like "test.txt" there will be no directory part, however we still need one, so insert './'
			const StringType directory = [&]() {
				const auto extracted_directory = UnderpinningString(path_string.begin(), pivot);
				return (extracted_directory.size() > 0) ? extracted_directory : UnderpinningString(_this_directory);
			}();
			const StringType filename = UnderpinningString(pivot, path_string.end());
			return PathParts(directory, filename);
		}

		bool pass_filter(const UnderpinningString& file_path)
		{ 
			if (_watching_single_file) {
				const UnderpinningString extracted_filename = { split_directory_and_file(file_path).filename };
				//if we are watching a single file, only that file should trigger action
				return extracted_filename == _filename;
			}
			return std::regex_match(file_path, _pattern);
		}

#ifdef _WIN32
		template<typename... Args> DWORD GetFileAttributesX(const char* lpFileName, Args... args) {
			return GetFileAttributesA(lpFileName, args...);
		}
		template<typename... Args> DWORD GetFileAttributesX(const wchar_t* lpFileName, Args... args) {
			return GetFileAttributesW(lpFileName, args...);
		}

		template<typename... Args> HANDLE CreateFileX(const char* lpFileName, Args... args) {
			return CreateFileA(lpFileName, args...);
		}
		template<typename... Args> HANDLE CreateFileX(const wchar_t* lpFileName, Args... args) {
			return CreateFileW(lpFileName, args...);
		}

		HANDLE get_directory(const StringType& path) 
		{
			auto file_info = GetFileAttributesX(path.c_str());

			if (file_info == INVALID_FILE_ATTRIBUTES)
			{
				throw std::system_error(GetLastError(), std::system_category());
			}
			_watching_single_file = (file_info & FILE_ATTRIBUTE_DIRECTORY) == false;

			const StringType watch_path = [this, &path]() {
				if (_watching_single_file)
				{
					const auto parsed_path = split_directory_and_file(path);
					_filename = parsed_path.filename;
					return parsed_path.directory;
				}
				else 
				{
					return path;
				}
			}();

			HANDLE directory = CreateFileX(
				watch_path.c_str(),           // pointer to the file name
				FILE_LIST_DIRECTORY,    // access (read/write) mode
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share mode
				nullptr, // security descriptor
				OPEN_EXISTING,         // how to create
				FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, // file attributes
				HANDLE(0));                 // file with attributes to copy

			if (directory == INVALID_HANDLE_VALUE)
			{
				throw std::system_error(GetLastError(), std::system_category());
			}
			return directory;
		}

		void convert_wstring(const std::wstring& wstr, std::string& out)
		{
			int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
			out.resize(size_needed, '\0');
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &out[0], size_needed, NULL, NULL);
		}

		void convert_wstring(const std::wstring& wstr, std::wstring& out)
		{
			out = wstr;
		}

		void monitor_directory() 
		{
			std::vector<BYTE> buffer(_buffer_size);
			DWORD bytes_returned = 0;
			OVERLAPPED overlapped_buffer{ 0 };

			overlapped_buffer.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (!overlapped_buffer.hEvent) {
				std::cerr << "Error creating monitor event" << std::endl;
			}

			std::array<HANDLE, 2> handles{ overlapped_buffer.hEvent, _close_event };

			auto async_pending = false;
			_running.set_value();
			do {
				std::vector<std::pair<StringType, Event>> parsed_information;
				ReadDirectoryChangesW(
					_directory,
					buffer.data(), static_cast<DWORD>(buffer.size()),
					TRUE,
					_listen_filters,
					&bytes_returned,
					&overlapped_buffer, NULL);
			
				async_pending = true;
			
				switch (WaitForMultipleObjects(2, handles.data(), FALSE, INFINITE))
				{
				case WAIT_OBJECT_0:
				{
					if (!GetOverlappedResult(_directory, &overlapped_buffer, &bytes_returned, TRUE)) {
						throw std::system_error(GetLastError(), std::system_category());
					}
					async_pending = false;

					if (bytes_returned == 0) {
						break;
					}

					FILE_NOTIFY_INFORMATION *file_information = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[0]);
					do
					{
						std::wstring changed_file_w{ file_information->FileName, file_information->FileNameLength / sizeof(file_information->FileName[0]) };
						UnderpinningString changed_file;
						convert_wstring(changed_file_w, changed_file);
						if (pass_filter(changed_file))
						{
							parsed_information.emplace_back(StringType{ changed_file }, _event_type_mapping.at(file_information->Action));
						}

						if (file_information->NextEntryOffset == 0) {
							break;
						}

						file_information = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(file_information) + file_information->NextEntryOffset);
					} while (true);
					break;
				}
				case WAIT_OBJECT_0 + 1:
					// quit
					break;
				case WAIT_FAILED:
					break;
				}
				//dispatch callbacks
				{
					std::lock_guard<std::mutex> lock(_callback_mutex);
					_callback_information.insert(_callback_information.end(), parsed_information.begin(), parsed_information.end());
				}
				_cv.notify_all();
			} while (_destory == false);

			if (async_pending)
			{
				//clean up running async io
				CancelIo(_directory);
				GetOverlappedResult(_directory, &overlapped_buffer, &bytes_returned, TRUE);
			}
		}
#endif // WIN32

#if __unix__

		bool is_file(const StringType& path) const
		{
			struct stat statbuf = {};
			if (stat(path.c_str(), &statbuf) != 0)
			{
				throw std::system_error(errno, std::system_category());
			}
			return S_ISREG(statbuf.st_mode);
		}

		FolderInfo get_directory(const StringType& path) 
		{
			const auto folder = inotify_init();
			if (folder < 0) 
			{
				throw std::system_error(errno, std::system_category());
			}

			_watching_single_file = is_file(path);

			const StringType watch_path = [this, &path]() {
				if (_watching_single_file)
				{
					const auto parsed_path = split_directory_and_file(path);
					_filename = parsed_path.filename;
					return parsed_path.directory;
				}
				else
				{
					return path;
				}
			}();

			const auto watch = inotify_add_watch(folder, watch_path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
			if (watch < 0) 
			{
				throw std::system_error(errno, std::system_category());
			}
			return { folder, watch };
		}

		void monitor_directory() 
		{
			std::vector<char> buffer(_buffer_size);

			_running.set_value();
			while (_destory == false) 
			{
				const auto length = read(_directory.folder, static_cast<void*>(buffer.data()), buffer.size());
				if (length > 0) 
				{
					int i = 0;
					std::vector<std::pair<StringType, Event>> parsed_information;
					while (i < length) 
					{
						struct inotify_event *event = reinterpret_cast<struct inotify_event *>(&buffer[i]); // NOLINT
						if (event->len) 
						{
							const UnderpinningString changed_file{ event->name };
							if (pass_filter(changed_file))
							{
								if (event->mask & IN_CREATE) 
								{
									parsed_information.emplace_back(StringType{ changed_file }, Event::added);
								}
								else if (event->mask & IN_DELETE) 
								{
									parsed_information.emplace_back(StringType{ changed_file }, Event::removed);
								}
								else if (event->mask & IN_MODIFY) 
								{
									parsed_information.emplace_back(StringType{ changed_file }, Event::modified);
								}
							}
						}
						i += event_size + event->len;
					}
					//dispatch callbacks
					{
						std::lock_guard<std::mutex> lock(_callback_mutex);
						_callback_information.insert(_callback_information.end(), parsed_information.begin(), parsed_information.end());
					}
					_cv.notify_all();
				}
			}
		}
#endif // __unix__

#if FILEWATCH_PLATFORM_MAC
            static StringType absolute_path_of(const StringType& path) {
                  char buf[PATH_MAX];
                  int fd = open((const char*)path.c_str(), O_RDONLY);
                  const char* str = buf;
                  struct stat stat;
                  mbstate_t state;

                  assert(fd != -1);
                  fcntl(fd, F_GETPATH, buf);
                  fstat(fd, &stat);

                  if (stat.st_mode & S_IFREG || stat.st_mode & S_IFLNK) {
                        size_t len = strlen(buf);

                        for (size_t i = len - 1; i >= 0; i--) {
                              if (buf[i] == '/') {
                                    buf[i] = '\0';
                                    break;
                              }
                        }
                  }
                  close(fd);

                  if (IsWChar<C>::value) {
                        size_t needed = mbsrtowcs(nullptr, &str, 0, &state) + 1;
                        StringType s;

                        s.reserve(needed);
                        mbsrtowcs((wchar_t*)&s[0], &str, s.size(), &state);
                        return s;
                  }
                  return StringType {buf};
            }
#elif defined(__unix__)
            static StringType absolute_path_of(const StringType& path) {
                  char buf[PATH_MAX];
                  const char* str = buf;
                  struct stat stat;
                  mbstate_t state;

                  realpath((const char*)path.c_str(), buf);
                  ::stat((const char*)path.c_str(), &stat);

                  if (stat.st_mode & S_IFREG || stat.st_mode & S_IFLNK) {
                        size_t len = strlen(buf);

                        for (size_t i = len - 1; i >= 0; i--) {
                              if (buf[i] == '/') {
                                    buf[i] = '\0';
                                    break;
                              }
                        }
                  }

                  if (IsWChar<C>::value) {
                        size_t needed = mbsrtowcs(nullptr, &str, 0, &state) + 1;
                        StringType s;

                        s.reserve(needed);
                        mbsrtowcs((wchar_t*)&s[0], &str, s.size(), &state);
                        return s;
                  }
                  return StringType {buf};
            }
#elif _WIN32
            static StringType absolute_path_of(const StringType& path) {
                  constexpr size_t size = IsWChar<C>::value? MAX_PATH : 32767 * sizeof(wchar_t);
                  char buf[size];

                  DWORD length = IsWChar<C>::value? 
                        GetFullPathNameW((LPCWSTR)path.c_str(), 
                              size / sizeof(TCHAR),
                              (LPWSTR)buf,
                              nullptr) : 
                        GetFullPathNameA((LPCSTR)path.c_str(), 
                              size / sizeof(TCHAR),
                              buf,
                              nullptr);
                  return StringType{(C*)buf, length};
            }
#endif

#if FILEWATCH_PLATFORM_MAC
            static StringType utf8StringToUtf32String(const char* buffer) {
                  mbstate_t state{};
                  StringType s{};

                  size_t needed = mbsrtowcs(nullptr, &buffer, 0, &state) + 1;
                  s.reserve(needed);
                  mbsrtowcs((wchar_t*)&s[0], &buffer, s.size(), &state);
                  return s;
            }

            template<typename Fn, class = std::enable_if<Invokable<Fn, StringType>::value>>
            static void walkDirectory(const StringType& path, Fn callback) {
                  int fd = open(path.c_str(), O_RDONLY);
                  char buf[1024];
                  long basep = 0;

                  if (fd == -1) {
                        return;
                  }

                  int ret = __getdirentries64(fd, buf, sizeof(buf), &basep);

                  while (ret > 0) {
                        char* current = buf;
                        int offset = 0;

                        while (offset < ret) {
                              struct dirent* dirent = (struct dirent*)current;
                              StringType name = IsWChar<C>::value? 
                                    utf8StringToUtf32String(dirent->d_name) 
                                    : StringType(dirent->d_name);
                              
                              callback(std::move(name));
                              current += dirent->d_reclen;
                              offset += dirent->d_reclen;
                        }
                        ret = __getdirentries64(fd, buf, sizeof(buf), &basep);
                  }
                  close(fd);
            }

            static StringType nameofFd(int fd) {
                  size_t len = 0;
                  char buf[MAXPATHLEN];

                  if (fcntl(fd, F_GETPATH, buf) == -1) {
                        return StringType{};
                  }
                  if (IsWChar<C>::value) {
                        return utf8StringToUtf32String(buf);
                  }

                  len = strnlen(buf, MAXPATHLEN);
                  for (int i = len - 1; i >= 0; i--) {
                        if(buf[i] == '/') {
                              return StringType{buf + i + 1, len - i - 1};
                        }
                  }
                  return StringType{buf, len};
            }

            static StringType fullPathOfFd(int fd) {
                  char buf[MAXPATHLEN];

                  if (fcntl(fd, F_GETPATH, buf) == -1) {
                        return StringType{};
                  }
                  if (IsWChar<C>::value) {
                        return utf8StringToUtf32String(buf);
                  }
                  return StringType{(C*)buf};
            }

            static StringType pathOfFd(int fd) {
                  size_t len = 0;
                  char buf[MAXPATHLEN];

                  if (fcntl(fd, F_GETPATH, buf) == -1) {
                        return StringType{};
                  }
                  if (IsWChar<C>::value) {
                        return utf8StringToUtf32String(buf);
                  }

                  len = strnlen(buf, MAXPATHLEN);
                  for (int i = len - 1; i >= 0; i--) {
                        if(buf[i] == '/') {
                              return StringType{buf, static_cast<size_t>(i)};
                        }
                  }
                  return StringType{buf, len};
            }

            static bool fdIsRemoved(int fd) {
                  char buf[MAXPATHLEN];
                  return fcntl(fd, F_GETPATH, buf) == -1;
            }

            FileState makeFileState(const StringType& path) {
                  int fd = openFile(path);
                  struct stat stat;

                  fstat(fd, &stat);

                  return FileState {
                        openFile(path),
                        stat.st_nlink,
                        stat.st_mtimespec.tv_sec
                  };
            }

            static StringType filenameOf(const StringType& file) {
                  for (int i = file.size() - 1; i >= 0; i--) {
                        if(file[i] == '/') {
                              return file.substr(i + 1);
                        }
                  }
                  return file;
            }

            static bool isInDirectory(const StringType& file, const StringType& path) {
                  if (file.size() < path.size()) {
                        return false;
                  }
                  return strncmp(file.data(), path.data(), path.size()) == 0;
            }

            PathParts splitPath(const StringType& path) {
                  PathParts split = split_directory_and_file(path);

                  if (split.directory.size() > 0 && split.directory[split.directory.size() - 1] == '/') {
                              split.directory.erase(split.directory.size() - 1);
                  }
                  return split;
            }

            StringType fullPathOf(const StringType& file) {
                  return _path + '/' + file;
            }

            int openFile(const StringType& file) {
                  int fd = open(fullPathOf(file).c_str(), O_RDONLY);
                  assert(fd != -1);
                  return fd;
            }

            void walkAndSeeChanges() {
                  struct RenamedPair {
                        StringType old;
                        StringType current;
                  };
                  struct EventInfo {
                        StringType file;
                        struct timespec time;
                        Event event;
                  };
                  std::unordered_map<StringType, FileState> newSnapshot{};
                  std::vector<EventInfo> events{};

                  for (auto& entry : _directory_snapshot) {
                        struct stat stat;

                        fstat(entry.second.fd, &stat);
                        if (fdIsRemoved(entry.second.fd)) {
                              events.push_back(EventInfo {
                                    .event = Event::removed,
                                    .file = entry.first,
                                    .time = stat.st_ctimespec
                              });
                              continue;
                        }

                        StringType fullPath = fullPathOfFd(entry.second.fd);
                        PathParts pathPair = splitPath(fullPath);

                        if (pathPair.directory != _path) {
                              events.push_back(EventInfo {
                                    .event = Event::removed,
                                    .file = entry.first,
                                    .time = stat.st_ctimespec
                              });
                              continue;
                        }
                        if (entry.first != pathPair.filename) {
                              events.push_back(EventInfo {
                                    .event = Event::renamed_old,
                                    .file = entry.first,
                                    .time = stat.st_ctimespec
                              });
                              events.push_back(EventInfo {
                                    .event = Event::renamed_new,
                                    .file = pathPair.filename,
                                    .time = stat.st_ctimespec
                              });
                        }
                        else {
                              if (stat.st_mtimespec.tv_sec > entry.second.last_modification) {
                                    entry.second.last_modification = stat.st_mtimespec.tv_sec;
                                    events.push_back(EventInfo {
                                          .event = Event::modified,
                                          .file = pathPair.filename,
                                          .time = stat.st_mtimespec
                                    });
                              }
                        }
                        newSnapshot.insert(std::make_pair(std::move(pathPair.filename), 
                              std::move(entry.second.invalidate_and_clone())));
                  }

                  walkDirectory(_path, [&](StringType file) {
                        if (isParentOrSelfDirectory(file) || !std::regex_match(file, _pattern)) {
                              return;
                        }
                        if (newSnapshot.count(file) == 0) {
                              FileState state = makeFileState(file);
                              struct stat stat;
                              
                              fstat(state.fd, &stat);
                              events.push_back(EventInfo {
                                    .event = Event::added,
                                    .file = file,
                                    .time = stat.st_mtimespec
                              });
                              newSnapshot.insert(std::make_pair(file, std::move(state)));
                        }
                  });

                  std::swap(_directory_snapshot, newSnapshot);

                  std::sort(events.begin(), events.end(), [] (const EventInfo& a, EventInfo& b) {
                        if (a.time.tv_sec == b.time.tv_sec) {
                              return a.time.tv_nsec < b.time.tv_nsec;
                        }
                        return a.time.tv_sec < b.time.tv_sec;
                  });

                  {
                        std::lock_guard<std::mutex> lock(_callback_mutex);
                        
                        for (const auto& event : events) {
                              _callback_information.push_back(std::make_pair(event.file, event.event));
                        }
                  }
                  _cv.notify_all();
            }

            void seeSingleFileChanges() {
                  struct EventInfo {
                        StringType file;
                        Event event;
                  };

                  int eventCount = 1;
                  EventInfo eventInfos[2];

                  if (fdIsRemoved(_file_fd)) {
                        eventInfos[0].event = Event::removed;
                        eventInfos[0].file = _filename;
                  }
                  else {
                        StringType absPath = pathOfFd(_file_fd);
                        PathParts split = splitPath(absPath);

                        if (split.directory != _path) {
                              eventInfos[0].event = Event::removed;
                              eventInfos[0].file = _filename;
                        }
                        else if (split.filename != _filename) {
                              eventInfos[0].event = Event::renamed_old;
                              eventInfos[0].file = std::move(_filename);
                              eventInfos[1].event = Event::renamed_new;
                              eventInfos[1].file = split.filename;
                              eventCount = 2;
                              _filename = std::move(split.filename);
                        }
                        else {
                              struct stat stat;

                              fstat(_file_fd, &stat);

                              if (stat.st_mtimespec.tv_sec > _last_modification_time.tv_sec) {
                                    eventInfos[0].event = Event::modified;
                                    eventInfos[0].file = _filename;
                                    _last_modification_time = stat.st_mtimespec;
                              }
                              else if (stat.st_mtimespec.tv_nsec > _last_modification_time.tv_nsec) {
                                    eventInfos[0].event = Event::modified;
                                    eventInfos[0].file = _filename;
                                    _last_modification_time = stat.st_mtimespec;
                              }
                              else {
                                    return;
                              }
                        }
                  }

                  {
                        std::lock_guard<std::mutex> lock(_callback_mutex);
                        for (int i = 0; i < eventCount; i++) {
                              _callback_information.push_back(
                                    std::make_pair(eventInfos[i].file, eventInfos[i].event));
                        }
                  }
                  _cv.notify_all();
            }

            void notify(CFStringRef path, const FSEventStreamEventFlags flags) {
                  CFIndex pathLength = CFStringGetLength(path);
                  CFIndex written = 0;
                  char buffer[PATH_MAX + 1];

                  CFStringGetBytes(path, 
                        CFRange {
                              .location = 0,
                              .length = pathLength,
                        }, 
                        IsWChar<C>::value? kCFStringEncodingUTF32 : kCFStringEncodingUTF8, 
                        0, 
                        false, 
                        (UInt8*)buffer, 
                        PATH_MAX, 
                        &written);
                  
                  buffer[written] = 0;

                  StringType absolutePath{(const C*)buffer, static_cast<size_t>(pathLength)};
                  PathParts pathPair = splitPath(absolutePath);

                  if (_watching_single_file && pathPair.filename != _filename) {
                        return;
                  }
                  if (pathPair.directory != _path || !std::regex_match(pathPair.filename, _pattern)) {
                        return;
                  }

                  Event event = Event::modified;
                  if (_previous_event_is_rename) {
                        event = Event::renamed_new;
                        _directory_snapshot.insert(std::make_pair(pathPair.filename, 
                              std::move(makeFileState(pathPair.filename))));
                        _previous_event_is_rename = false;
                  }
                  else if (flags & kFSEventStreamEventFlagItemRenamed) {
                        const auto state = _directory_snapshot.find(pathPair.filename);
                        assert(state != _directory_snapshot.end());
                        StringType fdPath = pathOfFd(state->second.fd);

                        // moved/delete to Trash folder
                        if (!isInDirectory(absolutePath, fdPath)) {
                              event = Event::removed;
                              _directory_snapshot.erase(pathPair.filename);
                        }
                        else {
                              event = Event::renamed_old;
                              _previous_event_is_rename = true;
                        }
                  }
                  else if (flags & kFSEventStreamEventFlagItemCreated) {
                        _directory_snapshot.insert(std::make_pair(pathPair.filename, 
                              std::move(makeFileState(pathPair.filename))));
                        event = Event::added;
                  }
                  else if (flags & kFSEventStreamEventFlagItemRemoved) {
                        _directory_snapshot.erase(pathPair.filename);
                        event = Event::removed;
                  }

                  {
                        std::lock_guard<std::mutex> lock(_callback_mutex);
                        _callback_information.push_back(std::make_pair(std::move(pathPair.filename), event));
                  }
                  _cv.notify_all();
            }

            static void handleFsEvent(__attribute__((unused)) ConstFSEventStreamRef streamFef, 
                                          void* clientCallBackInfo, 
                                          size_t numEvents, 
                                          CFArrayRef eventPaths, 
                                          const FSEventStreamEventFlags* eventFlags, 
                                          __attribute__((unused)) const FSEventStreamEventId* eventIds) {
                  FileWatch<StringType>* self = (FileWatch<StringType>*)clientCallBackInfo;

                  for (size_t i = 0; i < numEvents; i++) {
                        FSEventStreamEventFlags flag = eventFlags[i];
                        CFStringRef path = (CFStringRef)CFArrayGetValueAtIndex(eventPaths, i);

                        if (self->_watching_single_file) {
                              self->seeSingleFileChanges();
                        }
                        else if (flag & kFSEventStreamEventFlagMustScanSubDirs) {
                                    self->walkAndSeeChanges();
                        }
                        else {
                              self->notify(path, flag);
                        }
                  }
            }

            FSEventStreamRef openStream(const StringType& directory) {
                  CFStringEncoding encoding = IsWChar<C>::value? 
                        kCFStringEncodingUTF32 : kCFStringEncodingASCII;
                  CFStringRef path = CFStringCreateWithBytes(kCFAllocatorDefault, 
                                          (const UInt8*)directory.data(), 
                                          directory.size(), 
                                          encoding, 
                                          false);
                  CFArrayRef paths = CFArrayCreate(
                                          kCFAllocatorDefault, 
                                          (const void**)&path, 
                                          1, 
                                          nullptr);
                  FSEventStreamContext context {
                        .info = (void*)this
                  };
                  FSEventStreamRef event = FSEventStreamCreate(
                                    kCFAllocatorDefault, 
                                    (FSEventStreamCallback)handleFsEvent, 
                                    &context, 
                                    paths, 
                                    kFSEventStreamEventIdSinceNow, 
                                    0, 
                                    kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagFileEvents | 
                                    kFSEventStreamCreateFlagUseCFTypes);

                  CFRelease(path);
                  CFRelease(paths);
                  return event;
            }

            FSEventStreamRef openStreamForDirectory(const StringType& directory) {
                  FSEventStreamRef stream = openStream(directory);
                  walkDirectory(directory, [this] (StringType path) mutable {
                        if (!isParentOrSelfDirectory(path) && std::regex_match(path, _pattern)) {
                              _directory_snapshot.insert(std::make_pair(std::move(path), 
                                                std::move(makeFileState(path))));
                        }
                  });
                  return stream;
            }

            FSEventStreamRef openStreamForFile(const StringType& file) {
                  PathParts split = splitPath(file);

                  _watching_single_file = true;
                  _filename = std::move(split.filename);
                  _file_fd = openFile(file);
                  return openStreamForDirectory(split.directory);
            }

            FSEventStreamRef get_directory(const StringType& directory) {
                  struct stat stat;

                  ::stat((const char*)directory.c_str(), &stat);
                  if (stat.st_mode & S_IFDIR) {
                        return openStreamForDirectory(directory);
                  }
                  _last_modification_time = stat.st_mtimespec;
                  return openStreamForFile(directory);
            }

            void monitor_directory() {
                  _run_loop = CFRunLoopGetCurrent();
                  FSEventStreamScheduleWithRunLoop(_directory, 
                        _run_loop, 
                        kCFRunLoopDefaultMode);
                  FSEventStreamStart(_directory);
                  _running.set_value();
                  CFRunLoopRun();
            }
#endif // FILEWATCH_PLATFORM_MAC

		void callback_thread()
		{
			while (_destory == false) {
				std::unique_lock<std::mutex> lock(_callback_mutex);
				if (_callback_information.empty() && _destory == false) {
					_cv.wait(lock, [this] { return _callback_information.size() > 0 || _destory; });
				}
				decltype(_callback_information) callback_information = {};
				std::swap(callback_information, _callback_information);
				lock.unlock();

				for (const auto& file : callback_information) {
					if (_callback) {
						try
						{
							_callback(file.first, file.second);
						}
						catch (const std::exception&)
						{
						}
					}
				}
			}
		}
	};

	template<class StringType> constexpr typename FileWatch<StringType>::C FileWatch<StringType>::_regex_all[];
	template<class StringType> constexpr typename FileWatch<StringType>::C FileWatch<StringType>::_this_directory[];
}
#endif