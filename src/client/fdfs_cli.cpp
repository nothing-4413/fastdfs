#include<iostream>
#include<string>

/*
 * fdfs_cli.cpp
 *
 * 这是客户端命令行工具。
 *
 * 后续我们会支持：
 * 1. fdfs_cli upload <file>
 * 2. fdfs_cli download <file_id> <output>
 * 3. fdfs_cli delete <file_id>
 * 4. fdfs_cli stat
 *
 * FastDFS 风格的访问流程不是直接访问 storage。
 * 正确流程是：
 *
 * 上传：
 * client -> tracker 查询可用 storage
 * client -> storage 上传文件
 *
 * 下载：
 * client -> tracker 查询 file_id 对应的 storage
 * client -> storage 下载文件
 */

int main(int argc,char* argv[])
{
    /*
     * 至少需要一个命令参数。
     *
     * 例如：
     * ./fdfs_cli version
     * ./fdfs_cli upload ./test.jpg
     */

    if(argc < 2)
    {
        std::cerr << "usage: fdfs_cli <command> [args]" << std::endl;
        std::cerr << "commands:" << std::endl;
        std::cerr << "  version" << std::endl;
        std::cerr << "  upload <file>" << std::endl;
        std::cerr << "  download <file_id> <output>" << std::endl;
        return 1;
    }

    /*
     * argv[1] 是用户输入的命令。
     */
    std::string command = argv[1];

    /*
     * version 命令用于验证客户端程序是否能正常运行。
     */
    if(command == "version")
    {
        std::cout << "TinyFastDFS version 0.1.0" << std::endl;
        return 0;
    }

    /*
     * upload 命令当前只做参数检查和打印。
     * 后面会改成：
     * 1. 读取 client.conf
     * 2. 连接 tracker
     * 3. 查询可上传的 storage
     * 4. 连接 storage
     * 5. 发送文件内容
     * 6. 打印返回的 file_id
     */
    if(command == "upload")
    {
        if(argc < 3)
        {
            std::cerr << "usage: fdfs_cli upload <file>" << std::endl;
            return 1;
        }

        std::cout << "[client] upload file: " << argv[2] << std::endl;
        std::cout << "[client] upload is not implemented yet" << std::endl;
        return 0;
    }

    /*
     * download 命令当前只做参数检查和打印。
     * 后面会改成：
     * 1. 根据 file_id 查询 tracker
     * 2. tracker 返回可下载的 storage
     * 3. client 连接 storage
     * 4. storage 返回文件内容
     * 5. client 写入 output 文件
     */
    if (command == "download") {
        if (argc < 4) {
            std::cerr << "usage: fdfs_cli download <file_id> <output>" << std::endl;
            return 1;
        }

        std::cout << "[client] download file_id: " << argv[2] << std::endl;
        std::cout << "[client] output: " << argv[3] << std::endl;
        std::cout << "[client] download is not implemented yet" << std::endl;
        return 0;
    }

    /*
     * 如果命令不属于上面任何一种，说明用户输入了未知命令。
     */
    std::cerr << "unknown command: " << command << std::endl;
    return 1;
}