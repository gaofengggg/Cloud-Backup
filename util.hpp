
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
        std::string _filename;//�ļ�����
    public:
        FileUtil(const std::string& filename) :_filename(filename)//���캯�� 
        {}

        bool Remove()//ɾ���ļ�
        {
            //���ļ������� �൱��ɾ���ɹ� �򷵻�true
            if (this->Exists() == false)
            {
                return true;
            }
            remove(_filename.c_str());
            return true;
        }

        int64_t FileSize()//�ļ���С
        {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0)
            {
                std::cout << "get file size failed!\n";
                return -1;
            }
            return st.st_size;
        }

        time_t  LastMTime()//�ļ����һ���޸�ʱ��
        {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0)
            {
                std::cout << "get file size failed!\n";
                return -1;
            }
            return st.st_mtime;
        }

        time_t  LastATime()//�ļ����һ�η���ʱ��
        {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0)
            {
                std::cout << "get file size failed!\n";
                return -1;
            }
            return st.st_atime;
        }

        std::string FileName()//�ļ�����
        {
            // ./abc/test.txt
            size_t pos = _filename.find_last_of("/");
            if (pos == std::string::npos)
            {
                return _filename;
            }
            return _filename.substr(pos + 1);
        }
        bool GetPostLen(std::string* body, size_t pos, size_t len)//��ȡ�ļ�����
        {
            std::ifstream ifs;
            ifs.open(_filename, std::ios::binary);//�Զ����Ʒ�ʽ���ļ�
            if (ifs.is_open() == false)
            {
                std::cout << "read file failed" << std::endl;
                return false;
            }
            //�򿪳ɹ�����ȡ�ļ�����
            size_t fsize = this->FileSize();//��ȡ�ļ���С
            if (pos + len > fsize)//��pos��ʼλ�ó������ļ���С
            {
                std::cout << "get file len is error" << std::endl;
                return false;
            }
            ifs.seekg(pos, std::ios::beg);//���ļ���ʼλ��ƫ�Ƶ�posλ�ô�
            body->resize(len);
            ifs.read(&(*body)[0], len);//��ȡ�ļ��������ݵ� body��

            if (ifs.good() == false)//��ȡ����
            {
                std::cout << "get file content fialed " << std::endl;
                ifs.close();
                return false;
            }
            ifs.close();

            return true;
        }

        bool GetContent(std::string* body) //��ȡ�����ļ�����
        {
            size_t fsize = this->FileSize();//��ȡ�ļ���С
            return GetPostLen(body, 0, fsize);
        }

        bool SetContent(const std::string& body)//д���ļ�����
        {
            std::ofstream  ofs;
            ofs.open(_filename, std::ios::binary);  //�Զ����Ʒ�ʽ���ļ�
            if (ofs.is_open() == false)//��ʧ��
            {
                std::cout << "write open file failed" << std::endl;
                return false;
            }
            ofs.write(&body[0], body.size());//��body����д�뵽�ļ���
            if (ofs.good() == false)//д��ʧ��
            {
                std::cout << "write file content failed" << std::endl;
                ofs.close();
                return false;
            }

            ofs.close();
            return true;
        }

        bool Compress(const std::string& packname) //ѹ��
        {
            //��ȡ�ļ�����
            std::string body;
            if (this->GetContent(&body) == false)
            {
                std::cout << "compress get file content failed" << std::endl;
                return false;
            }
            //�����ݽ���ѹ��
            std::string packed = bundle::pack(bundle::LZIP, body);


            //��ѹ�����ݴ洢��ѹ�����ļ���
            FileUtil fu(packname);
            if (fu.SetContent(packed) == false)//д������ʧ��
            {
                std::cout << "compress write packed data failed" << std::endl;
                return false;
            }
            return true;
        }

        bool UnCompress(const std::string& filename)//��ѹ��
        {
            //����ǰѹ�������ݶ�ȡ����
            std::string body;
            if (this->GetContent(&body) == false)//��ȡ�ļ�����
            {
                std::cout << "compress get file content failed" << std::endl;
                return false;
            }

            //��ѹ�������ݽ��н�ѹ��
            std::string unpacked = bundle::unpack(body);

            //����ѹ��������д�뵽���ļ���
            FileUtil fu(filename);
            if (fu.SetContent(unpacked) == false)//д������ʧ��
            {
                std::cout << "uncompress write packed data failed" << std::endl;
                return false;
            }
            return true;
        }

        bool Exists()//�ж��ļ��Ƿ����
        {
            return fs::exists(_filename);
        }

        bool CreateDirectory()//����Ŀ¼
        {
            if (this->Exists())
            {
                return true;
            }
            return fs::create_directories(_filename);
        }

        bool ScanDirectory(std::vector<std::string>* arry)//���Ŀ¼
        {
            for (auto& p : fs::directory_iterator(_filename))//����Ŀ¼
            {
                if (fs::is_directory(p) == true)//�����������ļ� ��һ���ļ��� �Ͳ����в���
                {
                    continue;
                }
                //��ͨ�ļ��Ž��в���
                //relative_path ��ʾ����·�����ļ���
                arry->push_back(fs::path(p).relative_path().string());
            }
            return true;
        }
    };

    class JsonUtil
    {
    public:
        // ���л� ���ṹ������ ת��Ϊ�ַ���
        static bool Serialize(const Json::Value& root, std::string* str)
        {
            Json::StreamWriterBuilder swb;
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
            std::stringstream ss;
            sw->write(root, &ss);
            *str = ss.str();

            return true;
        }

        // �����л� ���ַ��� ת��Ϊ �ṹ������
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



