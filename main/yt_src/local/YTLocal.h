#ifndef __YT_LOCAL_H__
#define __YT_LOCAL_H__


enum YT_LOCAL_PLAY_TYPE
{
    YT_LOCAL_PLAY_TF,     //PLAY TF CARD MUSIC
    YT_LOCAL_PLAY_FLASH ,  //PLAY FLASH MUSIC
};


//--------------------FLASH MUSIC----------------------------------------------------------------
typedef struct FLASH_MUSIC_DATA{
    const char*  addr;
    unsigned int len;
}FlashMusicData;


typedef struct FLASH_MUSIC_INFO{
    unsigned int type;
    unsigned int count;
    FlashMusicData music_data[10];
}FlashMusicInfo;


enum YT_LOCAL_MODEL_TYPE
{
#if 1
	LOCAL_TYPE_MUSIC,		 //儿歌
	LOCAL_TYPE_STORY,		 //故事
	LOCAL_TYPE_GUOXUE,		 //国学
    LOCAL_TYPE_ENG_MUSIC,    //英语
#else

    LOCAL_TYPE_MUSIC,        //儿歌
    LOCAL_TYPE_STORY,        //故事
    LOCAL_TYPE_GUOXUE,       //国学
    LOCAL_TYPE_TANGSHI,      //唐诗
    LOCAL_TYPE_ENG_MUSIC,    //英语
    LOCAL_TYPE_FAVORITE,     //收藏
    LOCAL_TYPE_ZAOJIAO,      //早教
    LOCAL_TYPE_BAIKE,        //百科
    LOCAL_TYPE_YULE,         //娱乐
    LOCAL_TYPE_XIAOHUA,      //笑话
    
    LOCAL_TYPE_EGJX,         //儿歌精选
    LOCAL_TYPE_TSJX,         //唐诗精选
    LOCAL_TYPE_GXJD,         //国学经典
    LOCAL_TYPE_SQGS,         //睡前故事
    LOCAL_TYPE_THGS,         //童话故事
    LOCAL_TYPE_YYQM,         //英语启蒙
    LOCAL_TYPE_YYGS,         //英语故事
    LOCAL_TYPE_YYEG,         //英语儿歌
    LOCAL_TYPE_LXYY,         //流行音乐
    
    LOCAL_TYPE_EN_MUSIC,     //MUSIC
    LOCAL_TYPE_EN_STORY,     //STROY
    LOCAL_TYPE_EN_GUOXUE,    //GUOSUE
    LOCAL_TYPE_EN_TANGSHI,   //TANGSHI
    LOCAL_TYPE_EN_ENGLISH,   //ENGLISH
    LOCAL_TYPE_EN_FAVORITE,  //FAVORITE
    LOCAL_TYPE_EN_ENGMUSIC,  //ENGMUSIC


    LOCAL_TYPE_ROOT_DIR,    //root directory. END
#endif
    LOCAL_TYPE_END
};

// 注意：这个名子要与YT_LOCAL_MODEL_TYPE的对应一致


//--------------------FLASH MUSIC END------------------------------------------------------------


class PlayLocal
{
    public:
        static PlayLocal &instance();

        PlayLocal();
        virtual ~PlayLocal();

        void init();
        void reInit(YT_LOCAL_PLAY_TYPE type);
        void deinit();
        void initVar();

        bool checkTFCardDirExist();
        void getPrevFilePath(char* path);
        void getNextFilePath(char* path);
        void getFileByIndex(int index,char* path);

        void round_play();

        void switchLocalPlayModel(YT_LOCAL_MODEL_TYPE type);
        void switchLocalPlayPrevModel();		
        void switchLocalPlayNextModel();
        int getMaxModelCount();
        int getCurModelIndex();

        char* getModelString();

        void setLocalPlayType(YT_LOCAL_PLAY_TYPE type);
        YT_LOCAL_PLAY_TYPE getLocalPlayType();

        void switchPrevFileByFlash();
        void switchNextFileByFlash();

        char* getLocalFlashPlayAddr();
        unsigned int getLocalFlashPlayLen();


        void getPrevFileByFlashData(char **addr, unsigned int* len);
        void getNextFileByFlashData(char **addr, unsigned int* len);
		void getNextAlbumData(char **addr, unsigned int* len);
		void getPreAlbumData(char **addr, unsigned int* len);

        void setLoopAllAudio(bool bLoop);
        bool getLoopAllAudio();



    private:
        void initDir();
        char* getModelDirString();
        void playModelTipSound();

        void switchPrev();
        void switchNext();

        int initTFFileInfo();
        int initFlashFileInfo();

		int initSpiFlashFileInfo();
        bool isMp3(char* name);

    private:	
        static PlayLocal s_instance;

        //FILE *ftmp;
        int CurIndex;
        int MaxFiles;

        YT_LOCAL_PLAY_TYPE nPlayType;
        int nCurModelIndex;
        int nModelMaxFiles[LOCAL_TYPE_END]; //

        bool bLoopAllAudio;

};


#endif
