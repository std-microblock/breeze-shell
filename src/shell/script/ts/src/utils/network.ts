import * as shell from "mshell"

export const get_async = url => {
    url = url.replaceAll('//', '/').replaceAll(':/', '://')
    shell.println(url)
    return new Promise((resolve, reject) => {
        shell.network.get_async(encodeURI(url), data => {
            resolve(data)
        }, err => {
            reject(err)
        })
    })
}