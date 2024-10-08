
#ifndef  _MY_SERVICE_//����ͷ�ļ��ظ�����
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

        static void Upload(const httplib::Request& req, httplib::Response& rsp)//�ϴ�����
        {
            // post /upload �ļ�������������
            auto ret = req.has_file("file");//�ж���û���ϴ����ļ�����
            if (ret == false)
            {
                rsp.status = 400;
                return;
            }
            //�����ϴ����ļ����� ���ȡ�ļ������ĸ�������
            const auto& file = req.get_file_value("file");

            //file.filename�ļ�����   file.content �ļ�����
            std::string back_dir = Config::GetInstance()->GetBackDir();
            std::string realpath = back_dir + FileUtil(file.filename).FileName();

            FileUtil fu(realpath);
            fu.SetContent(file.content);//������д���ļ���

            BackupInfo info;
            info.NewBackupInfo(realpath);//��֯�����ļ���Ϣ
            _data->Insert(info);//�����ݹ���ģ����ӱ����ļ���Ϣ
            return;
        }


        static std::string TimetoStr(time_t  t)
        {
            std::string tmp = std::ctime(&t);
            return tmp;

        }
        static void ListShow(const httplib::Request& req, httplib::Response& rsp)// ��ȡչʾҳ�洦��
        {
            //��ȡ�����ļ���Ϣ
            std::vector<BackupInfo> arry;
            _data->GetAll(&arry);

            //�������б�����Ϣ ��֯html�ļ�����
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
            rsp.set_header("Content-Type", "text/html");//����ͷ����Ϣ
            rsp.status = 200;//״̬������Ϊ200 ��ʾ�ɹ���Ӧ
            return;
        }



        static std::string GetETag(const  BackupInfo& info)//��ȡ ETag�ֶ�
        {
            //  etag �ļ��� - filename-fsize �ļ���С - mtime ���һ���޸�ʱ�� 
            FileUtil fu(info.real_path);
            std::string etag = fu.FileName();//��ȡ�ļ���
            etag += "-";
            etag += std::to_string(info.fsize);//��ȡ�ļ���С(�ַ�����ʽ)
            etag += "-";
            etag += std::to_string(info.mtime);//��ȡ���һ���޸�ʱ��(�ַ�����ʽ) 
            return etag;
        }

        static void Download(const httplib::Request& req, httplib::Response& rsp)//�ļ���������
        {
            // ��ȡ�ͻ����������Դ·�� req.path

            // ������Դ·��  ��ȡ�ļ�������Ϣ
            BackupInfo info;
            _data->GetOneByURL(req.path, &info);//ͨ��URL��ȡ��������

            //�ж��ļ��Ƿ�ѹ��  �����ѹ�� ��Ҫ�Ƚ�ѹ��
            if (info.pack_flag == true)//˵����ѹ����
            {
                FileUtil fu(info.pack_path); //ʹ��ѹ����·��ʵ����һ������
                fu.UnCompress(info.real_path);//��ѹ��

                 // ɾ��ѹ����  �޸ı�����Ϣ
                fu.Remove();//ɾ���ļ�
                info.pack_flag = false;
                _data->Update(info);//����
            }

            // ��ȡ�ļ����� ���� rsp.body��
            FileUtil fu(info.real_path);
            fu.GetContent(&rsp.body);//�� real_path�е����� ���� body��

            bool retrans = false;//retrans��ʾ�ϵ�����
            std::string old_etag;
            if (req.has_header("If-Range"))//�����ڸ�ͷ���ֶ� ��˵��Ϊ�ϵ�����
            {
                old_etag = req.get_header_value("If-Range");//��ͷ���ֶζ�Ӧ��valueֵ���� old_etag
                if (old_etag == GetETag(info))//��If-Range�ֶ� ��ֵ �������ļ�������etagһ�� ����϶ϵ�����
                {
                    retrans = true;
                }
            }

            //��û�� If-Range �ֶ� ��Ϊ��������
            //���������ֶ� ��������ֵ�뵱ǰ�ļ��� etag ��һ�� ����뷵��ȫ������
            if (retrans == false)
            {
                //��������
                fu.GetContent(&rsp.body);//�� real_path�е����� ���� body��
                //������Ӧͷ���ֶ�  ETag  Accept-Ranges: bytes
                rsp.set_header("Accept-Ranges", "bytes");//����ͷ���ֶ�
                rsp.set_header("ETag", GetETag(info));
                rsp.set_header("Content-Type", "application/octet-stream");
                rsp.status = 200;//��Ӧ�ɹ�
            }
            else
            {
                //˵���Ƕϵ����� 
                //httplib �ڲ�ʵ�ֶ��� �ϵ���������Ĵ���
                //ֻ��Ҫ�û� ���ļ��������ݶ�ȡ��rsp.body��
                //�ڲ����Զ������������� ��body��ȡ��ָ������ ���� ������Ӧ
                fu.GetContent(&rsp.body);//�� real_path�е����� ���� body��
                rsp.set_header("Accept-Ranges", "bytes");//����ͷ���ֶ�
                rsp.set_header("ETag", GetETag(info));
                rsp.set_header("Content-Type", "application/octet-stream");
                rsp.status = 206;  //����������Ӧ  
            }
        }

    public:
        Service()//���캯��
        {
            Config* config = Config::GetInstance();//��������
            _server_port = config->GetServerPort();
            _server_ip = config->GetServerIp();
            _download_prefix = config->GetDownloadPrefix();
        }
        bool RunModule()//����ģ��
        {
            _server.Post("/upload", Upload);
            _server.Get("/listshow", ListShow);
            _server.Get("/", ListShow);
            std::string download_url = _download_prefix + "(.*)";//��������
            _server.Get(download_url, Download);

            _server.listen(_server_ip.c_str(), _server_port);
            return true;
        }
    };
}
#endif 


