#pragma once

#include <cstdint>

/*
 * Command
 *
 * 这里定义 TinyFastDFS 的协议命令号。
 *
 * FastDFS 官方协议中，不同操作由不同 cmd 表示：
 * 例如 storage join、heartbeat、upload、download。
 *
 * 我们这里不照抄官方命令号，而是重新设计一套更适合学习的命令号。
 * 但是思想保持一致：
 *
 * cmd 决定服务端应该执行什么操作。
 */

 enum class Command : uint8_t
 {
    /*
     * PING 用来测试网络和协议是否打通。
     *
     * 当前 Step 4 只实现 PING。
     */
    PING = 1;

     /*
     * storage 启动后向 tracker 注册自己。
     *
     * 后面 Step 5 会实现。
     */
    STORAGE_JOIN = 10;

    /*
     * storage 定期向 tracker 发送心跳。
     *
     * 后面 Step 6 会实现。
     */
    STORAGE_HEARTBEAT = 11;

    /*
     * client 上传前向 tracker 查询可用 storage。
     */
    QUERY_UPLOAD_STORAGE = 20;

    /*
     * client 下载前向 tracker 查询文件所在 storage。
     */
    QUERY_DOWNLOAD_STORAGE = 21;
    
    /*
    * client 上传成功后，向 tracker 汇报文件实际落在哪个 storage。
    *
    * 这一步用于建立：
    * file_id -> storage
    *
    * 后续下载 / 删除 / stat 时，tracker 就能返回正确 storage。
    */
    REPORT_FILE_UPLOAD = 22,

    /*
     * client 向 storage 上传文件。
     */
    UPLOAD_FILE = 30;

    /*
     * client 从 storage 下载文件。
     */
    DOWNLOAD_FILE = 31;

     /*
     * client 删除 storage 上的文件。
     */
    DELETE_FILE = 32;

    /*
     * 通用响应命令。
     *
     * 服务端收到请求后，通常返回 RESPONSE。
     */
    RESPONSE = 255;
 };

 /*
 * Status
 *
 * 表示响应状态。
 *
 * OK 表示成功。
 * ERROR 表示失败。
 */
enum class Status : uint8_t
{
    OK = 0;
    ERROR = 1;
};