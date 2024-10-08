
#ifndef _MY_UTIL_
#define _MY_UTIL_
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<sys/stat.h>
#include"bundle.h"
#include<experimental/filesystem>
#include<jsoncpp/json/json.h>
#include<memory>

namespace cloud
{
    namespace fs = std::experimental::filesystem;

    class FileUtil
    {
    private:
        std::string _filename;//文件名称
    public:
        FileUtil(const std::string& filename) :_filename(filename)//构造函数 
        {}

        bool Remove()//删除文件
        {
            //若文件不存在 相当于删除成功 则返回true
            if (this->Exists() == false)
            {
                return true;
            }
            remove(_filename.c_str());
            return true;
        }

        int64_t FileSize()//文件大小
        {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0)
            {
                std::cout << "get file size failed!\n";
                return -1;
            }
            return st.st_size;
        }

        time_t  LastMTime()//文件最后一次修改时间
        {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0)
            {
                std::cout << "get file size failed!\n";
                return -1;
            }
            return st.st_mtime;
        }

        time_t  LastATime()//文件最后一次访问时间
        {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0)
            {
                std::cout << "get file size failed!\n";
                return -1;
            }
            return st.st_atime;
        }

        std::string FileName()//文件名称
        {
            // ./abc/test.txt
            size_t pos = _filename.find_last_of("/");
            if (pos == std::string::npos)
            {
                return _filename;
            }
            return _filename.substr(pos + 1);
        }
        bool GetPostLen(std::string* body, size_t pos, size_t len)//获取文件数据
        {
            std::ifstream ifs;
            ifs.open(_filename, std::ios::binary);//以二进制方式打开文件
            if (ifs.is_open() == false)
            {
                std::cout << "read file failed" << std::endl;
                return false;
            }
            //打开成功，获取文件数据
            size_t fsize = this->FileSize();//获取文件大小
            if (pos + len > fsize)//若pos开始位置超过了文件大小
            {
                std::cout << "get file len is error" << std::endl;
                return false;
            }
            ifs.seekg(pos, std::ios::beg);//从文件起始位置偏移到pos位置处
            body->resize(len);
            ifs.read(&(*body)[0], len);//读取文件所有数据到 body中

            if (ifs.good() == false)//读取出错
            {
                std::cout << "get file content fialed " << std::endl;
                ifs.close();
                return false;
            }
            ifs.close();

            return true;
        }

        bool GetContent(std::string* body) //获取整体文件数据
        {
            size_t fsize = this->FileSize();//获取文件大小
            return GetPostLen(body, 0, fsize);
        }

        bool SetContent(const std::string& body)//写入文件数据
        {
            std::ofstream  ofs;
            ofs.open(_filename, std::ios::binary);  //以二进制方式打开文件
            if (ofs.is_open() == false)//打开失败
            {
                std::cout << "write open file failed" << std::endl;
                return false;
            }
            ofs.write(&body[0], body.size());//将body数据写入到文件中
            if (ofs.good() == false)//写入失败
            {
                std::cout << "write file content failed" << std::endl;
                ofs.close();
                return false;
            }

            ofs.close();
            return true;
        }

        bool Compress(const std::string& packname) //压缩
        {
            //读取文件数据
            std::string body;
            if (this->GetContent(&body) == false)
            {
                std::cout << "compress get file content failed" << std::endl;
                return false;
            }
            //对数据进行压缩
            std::string packed = bundle::pack(bundle::LZIP, body);


            //将压缩数据存储到压缩包文件中
            FileUtil fu(packname);
            if (fu.SetContent(packed) == false)//写入数据失败
            {
                std::cout << "compress write packed data failed" << std::endl;
                return false;
            }
            return true;
        }

        bool UnCompress(const std::string& filename)//解压缩
        {
            //将当前压缩包数据读取出来
            std::string body;
            if (this->GetContent(&body) == false)//获取文件内容
            {
                std::cout << "compress get file content failed" << std::endl;
                return false;
            }

            //对压缩的数据进行解压缩
            std::string unpacked = bundle::unpack(body);

            //将解压缩的数据写入到新文件中
            FileUtil fu(filename);
            if (fu.SetContent(unpacked) == false)//写入数据失败
            {
                std::cout << "uncompress write packed data failed" << std::endl;
                return false;
            }
            return true;
        }

        bool Exists()//判断文件是否存在
        {
            return fs::exists(_filename);
        }

        bool CreateDirectory()//创建目录
        {
            if (this->Exists())
            {
                return true;
            }
            return fs::create_directories(_filename);
        }

        bool ScanDirectory(std::vector<std::string>* arry)//浏览目录
        {
            for (auto& p : fs::directory_iterator(_filename))//遍历目录
            {
                if (fs::is_directory(p) == true)//检测遍历到的文件 是一个文件夹 就不进行操作
                {
                    continue;
                }
                //普通文件才进行操作
                //relative_path 表示带有路径的文件名
                arry->push_back(fs::path(p).relative_path().string());
            }
            return true;
        }
    };

    class JsonUtil
    {
    public:
        // 序列化 将结构化数据 转化为字符串
        static bool Serialize(const Json::Value& root, std::string* str)
        {
            Json::StreamWriterBuilder swb;
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
            std::stringstream ss;
            sw->write(root, &ss);
            *str = ss.str();

            return true;
        }

        // 反序列化 将字符串 转化为 结构化数据
        static bool UnSerialize(const std::string& str, Json::Value* root)
        {
            Json::CharReaderBuilder crb;
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
            std::string err;
            bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), root, &err);
            if (ret == false)
            {
                std::cout << "parse error:" << err << std::endl;
                return false;
            }
            return true;
        }
    };
}

#endif



