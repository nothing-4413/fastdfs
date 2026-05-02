#include<iostream>
#include<string>

/*
 * storage_main.cpp
 *
 * 这个文件是 storage 进程的入口。
 *
 * 在 FastDFS 架构中，storage 是真正存文件的节点。
 * 它负责：
 * 1. 启动后向 tracker 注册自己
 * 2. 定期向 tracker 发送心跳
 * 3. 接收客户端上传的文件
 * 4. 根据 file_id 返回文件内容
 * 5. 删除文件
 * 6. 后期与同 group 内其他 storage 做副本同步
 *
 */

 int main(int argc,char* argv[])
 {
    /*
     * storage.conf 里会保存：
     * group_name
     * port
     * base_path
     * store_path0
     * tracker_server
     */

     if(argc < 2)
     {
        std::cerr << "usage: storage_server <storage_conf>" << std::endl;
        return 1;
     }

    /*
     * 保存 storage 配置文件路径。
     * 下一步会实现 Config 模块，然后真正读取里面的 group_name、port 等配置。
     */

    std::string conf_path = argv[1];

    std::cout << "[storage] starting..." << std::endl;
    std::cout << "[storage] config: " << conf_path << std::endl;
    std::cout << "[storage] role: storage server" << std::endl;
    std::cout << "[storage] status: initialized" << std::endl;

    return 0;
 }