
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
        std::string _back_dir;// 备份文件路径
        std::string _pack_dir;// 压缩文件路径
        std::string _pack_suffix;//压缩包后缀名
        int _hot_time; // 热点时间

    private:
        bool Hotjudge(const std::string& filename)//热点判断
        {
            FileUtil fu(filename);
            time_t last_atime = fu.LastATime();//文件最后一次访问时间
            time_t cur_time = time(NULL);//当前系统时间
            if (cur_time - last_atime > _hot_time)
            {
                //差值超过设定的值 所以为非热点文件
                return true;
            }
            //差值小于设定的值 所以为热点文件
            return false;
        }


    public:
        HotManager() //构造函数
        {
            Config* config = Config::GetInstance();//创建对象
            _back_dir = config->GetBackDir();
            _pack_dir = config->GetPackDir();
            _pack_suffix = config->GetPackFileSuffix();
            _hot_time = config->GetHotTime();
            FileUtil tmp1(_back_dir);
            FileUtil tmp2(_pack_dir);
            //为了防止目录不存在 则创建目录
            tmp1.CreateDirectory();//创建目录
            tmp2.CreateDirectory();

        }

        bool RunModule()//运行模块
        {
            while (1)
            {
                //遍历备份目录 获取所有文件名
                FileUtil fu(_back_dir);
                std::vector<std::string>arry;
                fu.ScanDirectory(&arry);

                //遍历判断文件是否是非热点文件
                for (auto& a : arry)
                {
                    if (Hotjudge(a) == false)//说明为热点文件
                    {
                        //热点文件不需要处理 所以直接跳过
                        continue;
                    }

                    //获取文件备份信息
                    cloud::BackupInfo bi;
                    //GetOneByRealPat 通过realpath获取单个数据
                    if (_data->GetOneByRealPath(a, &bi) == false)
                    {
                        //有文件存在 但是没有备份信息
                        bi.NewBackupInfo(a);
                    }

                    //对非热点文件进行压缩处理
                    FileUtil tmp(a);
                    tmp.Compress(bi.pack_path);//压缩

                    //删除源文件 修改备份信息
                    tmp.Remove();
                    bi.pack_flag = true;//表示压缩成功
                    _data->Update(bi);//将bi更新到_table哈希表中
                }
                usleep(1000);
            }
            return true;
        }
    };

}

#endif

