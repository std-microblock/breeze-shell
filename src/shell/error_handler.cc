#include "error_handler.h"

#include <string>

#include "build_info.h"
#include "config.h"
#include "utils.h"

#include "sentry.h"
#include "wintoastlib.h"

void show_crash_toast() {
    using namespace WinToastLib;

    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        WinToast::instance()->setAppName(L"Breeze");
        WinToast::instance()->setAppUserModelId(L"breeze-shell");
        WinToast::instance()->initialize();
    }

    WinToastTemplate templ(WinToastTemplate::ImageAndText02);
    templ.setTextField(L"Explorer 崩溃了", WinToastTemplate::FirstLine);
    templ.setTextField(L"这可能是 Breeze Shell 或其他 Shell Extension / "
                       L"插件造成的。崩溃日志已发送。",
                       WinToastTemplate::SecondLine);

    static struct WinToastEventHandler : public IWinToastHandler {
        void toastActivated() const override {}
        void toastActivated(int actionIndex) const override {}
        void toastActivated(const char *) const override {}
        void toastDismissed(WinToastDismissalReason state) const override {}
        void toastFailed() const override {}
    } handler;
    WinToast::instance()->showToast(templ, &handler);
}

void mb_shell::install_error_handlers() {
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options,
                           "https://"
                           "159f800f906ad4d9e04d139d5db21fbd@o4510630644744192."
                           "ingest.de.sentry.io/4510987356274768");

    sentry_options_set_release(options, "breeze-shell@" BREEZE_VERSION);
    sentry_options_set_environment(options, "production");

    auto db_path = config::data_directory() / ".sentry-native";
    sentry_options_set_database_path(options, db_path.string().c_str());

    sentry_options_set_handler_path(options, nullptr);

    sentry_options_set_before_send(
        options,
        [](sentry_value_t event, void *hint, void *closure) -> sentry_value_t {
            show_crash_toast();
            return event;
        },
        nullptr);
    sentry_init(options);
    sentry_set_tag("git_commit", BREEZE_GIT_COMMIT_HASH);
    sentry_set_tag("git_branch", BREEZE_GIT_BRANCH_NAME);
    sentry_set_tag("build_date", BREEZE_BUILD_DATE_TIME);
    sentry_set_tag("windows_version",
                   mb_shell::is_win11_or_later() ? "11+" : "10-");
}
