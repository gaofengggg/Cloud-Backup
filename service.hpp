
#ifndef  _MY_SERVICE_//避免头文件重复包含
#define  _MY_SERVICE_
#include "data.hpp"
#include "httplib.h"

extern cloud::DataManager* _data;
namespace cloud
{
    class Service
    {
    private:
        int _server_port;
        std::string _server_ip;
        std::string _download_prefix;
        httplib::Server _server;

    private:

        static void Upload(const httplib::Request& req, httplib::Response& rsp)//上传请求
        {
            // post /upload 文件数据在正文中
            auto ret = req.has_file("file");//判断有没有上传的文件区域
            if (ret == false)
            {
                rsp.status = 400;
                return;
            }
            //若有上传的文件区域 则获取文件包含的各项数据
            const auto& file = req.get_file_value("file");

            //file.filename文件名称   file.content 文件内容
            std::string back_dir = Config::GetInstance()->GetBackDir();
            std::string realpath = back_dir + FileUtil(file.filename).FileName();

            FileUtil fu(realpath);
            fu.SetContent(file.content);//将数据写入文件中

            BackupInfo info;
            info.NewBackupInfo(realpath);//组织备份文件信息
            _data->Insert(info);//向数据管理模块添加备份文件信息
            return;
        }


        static std::string TimetoStr(time_t  t)
        {
            std::string tmp = std::ctime(&t);
            return tmp;

        }
        static void ListShow(const httplib::Request& req, httplib::Response& rsp)// 获取展示页面处理
        {
            //获取所有文件信息
            std::vector<BackupInfo> arry;
            _data->GetAll(&arry);

            //根据所有备份信息 组织html文件数据
            std::stringstream ss;
            ss << "<html><head><title>Download</title><meta charset='UTF-8'></head>";
            ss << "<body><hl>Download</hl><table>";
            for (auto& a : arry)
            {
                ss << "<tr>";
                std::string filename = FileUtil(a.real_path).FileName();
                ss << "<td><a href =' " << a.url << " '>" << filename << "</a></td>";
                ss << "<td align='right'>" << TimetoStr(a.mtime) << "</td>";
                ss << "<td align ='right'>" << a.fsize / 1024 << "k</td>";
                ss << "</tr>";
            }
            ss << "</table></body></html>";
            rsp.body = ss.str();
            rsp.set_header("Content-Type", "text/html");//设置头部信息
            rsp.status = 200;//状态码设置为200 表示成功响应
            return;
        }



        static std::string GetETag(const  BackupInfo& info)//获取 ETag字段
        {
            //  etag 文件名 - filename-fsize 文件大小 - mtime 最后一次修改时间 
            FileUtil fu(info.real_path);
            std::string etag = fu.FileName();//获取文件名
            etag += "-";
            etag += std::to_string(info.fsize);//获取文件大小(字符串格式)
            etag += "-";
            etag += std::to_string(info.mtime);//获取最后一次修改时间(字符串格式) 
            return etag;
        }

        static void Download(const httplib::Request& req, httplib::Response& rsp)//文件下载请求
        {
            // 获取客户端请求的资源路径 req.path

            // 根据资源路径  获取文件备份信息
            BackupInfo info;
            _data->GetOneByURL(req.path, &info);//通过URL获取单个数据

            //判断文件是否被压缩  如果被压缩 需要先解压缩
            if (info.pack_flag == true)//说明被压缩了
            {
                FileUtil fu(info.pack_path); //使用压缩包路径实例化一个对象
                fu.UnCompress(info.real_path);//解压缩

                 // 删除压缩包  修改备份信息
                fu.Remove();//删除文件
                info.pack_flag = false;
                _data->Update(info);//更新
            }

            // 读取文件数据 放入 rsp.body中
            FileUtil fu(info.real_path);
            fu.GetContent(&rsp.body);//将 real_path中的数据 放入 body中

            bool retrans = false;//retrans表示断点续传
            std::string old_etag;
            if (req.has_header("If-Range"))//若存在该头部字段 则说明为断点续传
            {
                old_etag = req.get_header_value("If-Range");//将头部字段对应的value值传给 old_etag
                if (old_etag == GetETag(info))//若If-Range字段 的值 与请求文件的最新etag一致 则符合断点续传
                {
                    retrans = true;
                }
            }

            //若没有 If-Range 字段 则为正常下载
            //如果有这个字段 但是它的值与当前文件的 etag 不一致 则必须返回全部数据
            if (retrans == false)
            {
                //正常下载
                fu.GetContent(&rsp.body);//将 real_path中的数据 放入 body中
                //设置响应头部字段  ETag  Accept-Ranges: bytes
                rsp.set_header("Accept-Ranges", "bytes");//建立头部字段
                rsp.set_header("ETag", GetETag(info));
                rsp.set_header("Content-Type", "application/octet-stream");
                rsp.status = 200;//响应成功
            }
            else
            {
                //说明是断点续传 
                //httplib 内部实现对于 断点续传请求的处理
                //只需要用户 将文件所有数据读取到rsp.body中
                //内部会自动根据请求区间 从body中取出指定区间 数据 进行响应
                fu.GetContent(&rsp.body);//将 real_path中的数据 放入 body中
                rsp.set_header("Accept-Ranges", "bytes");//建立头部字段
                rsp.set_header("ETag", GetETag(info));
                rsp.set_header("Content-Type", "application/octet-stream");
                rsp.status = 206;  //区间请求响应  
            }
        }

    public:
        Service()//构造函数
        {
            Config* config = Config::GetInstance();//创建对象
            _server_port = config->GetServerPort();
            _server_ip = config->GetServerIp();
            _download_prefix = config->GetDownloadPrefix();
        }
        bool RunModule()//运行模块
        {
            _server.Post("/upload", Upload);
            _server.Get("/listshow", ListShow);
            _server.Get("/", ListShow);
            std::string download_url = _download_prefix + "(.*)";//下载请求
            _server.Get(download_url, Download);

            _server.listen(_server_ip.c_str(), _server_port);
            return true;
        }
    };
}
#endif 


