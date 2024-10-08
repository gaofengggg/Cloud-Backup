
#ifndef _MY_HOT_
#define _MY_HOT_
#include<unistd.h>
#include"data.hpp"


extern cloud::DataManager* _data;
namespace cloud
{
    class HotManager
    {
    private:
        std::string _back_dir;// �����ļ�·��
        std::string _pack_dir;// ѹ���ļ�·��
        std::string _pack_suffix;//ѹ������׺��
        int _hot_time; // �ȵ�ʱ��

    private:
        bool Hotjudge(const std::string& filename)//�ȵ��ж�
        {
            FileUtil fu(filename);
            time_t last_atime = fu.LastATime();//�ļ����һ�η���ʱ��
            time_t cur_time = time(NULL);//��ǰϵͳʱ��
            if (cur_time - last_atime > _hot_time)
            {
                //��ֵ�����趨��ֵ ����Ϊ���ȵ��ļ�
                return true;
            }
            //��ֵС���趨��ֵ ����Ϊ�ȵ��ļ�
            return false;
        }


    public:
        HotManager() //���캯��
        {
            Config* config = Config::GetInstance();//��������
            _back_dir = config->GetBackDir();
            _pack_dir = config->GetPackDir();
            _pack_suffix = config->GetPackFileSuffix();
            _hot_time = config->GetHotTime();
            FileUtil tmp1(_back_dir);
            FileUtil tmp2(_pack_dir);
            //Ϊ�˷�ֹĿ¼������ �򴴽�Ŀ¼
            tmp1.CreateDirectory();//����Ŀ¼
            tmp2.CreateDirectory();

        }

        bool RunModule()//����ģ��
        {
            while (1)
            {
                //��������Ŀ¼ ��ȡ�����ļ���
                FileUtil fu(_back_dir);
                std::vector<std::string>arry;
                fu.ScanDirectory(&arry);

                //�����ж��ļ��Ƿ��Ƿ��ȵ��ļ�
                for (auto& a : arry)
                {
                    if (Hotjudge(a) == false)//˵��Ϊ�ȵ��ļ�
                    {
                        //�ȵ��ļ�����Ҫ���� ����ֱ������
                        continue;
                    }

                    //��ȡ�ļ�������Ϣ
                    cloud::BackupInfo bi;
                    //GetOneByRealPat ͨ��realpath��ȡ��������
                    if (_data->GetOneByRealPath(a, &bi) == false)
                    {
                        //���ļ����� ����û�б�����Ϣ
                        bi.NewBackupInfo(a);
                    }

                    //�Է��ȵ��ļ�����ѹ������
                    FileUtil tmp(a);
                    tmp.Compress(bi.pack_path);//ѹ��

                    //ɾ��Դ�ļ� �޸ı�����Ϣ
                    tmp.Remove();
                    bi.pack_flag = true;//��ʾѹ���ɹ�
                    _data->Update(bi);//��bi���µ�_table��ϣ����
                }
                usleep(1000);
            }
            return true;
        }
    };

}

#endif

