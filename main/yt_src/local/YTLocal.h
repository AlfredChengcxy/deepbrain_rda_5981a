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
	LOCAL_TYPE_MUSIC,		 //����
	LOCAL_TYPE_STORY,		 //����
	LOCAL_TYPE_GUOXUE,		 //��ѧ��
    LOCAL_TYPE_ENG_MUSIC,    //Ӣ�ﯭ
#else

    LOCAL_TYPE_MUSIC,        //����
    LOCAL_TYPE_STORY,        //����
    LOCAL_TYPE_GUOXUE,       //��ѧ��
    LOCAL_TYPE_TANGSHI,      //��ʫ
    LOCAL_TYPE_ENG_MUSIC,    //Ӣ�ﯭ
    LOCAL_TYPE_FAVORITE,     //�ղ�
    LOCAL_TYPE_ZAOJIAO,      //���
    LOCAL_TYPE_BAIKE,        //�ٿ�
    LOCAL_TYPE_YULE,         //����
    LOCAL_TYPE_XIAOHUA,      //Ц��
    
    LOCAL_TYPE_EGJX,         //���辫ѡ
    LOCAL_TYPE_TSJX,         //��ʫ��ѡ
    LOCAL_TYPE_GXJD,         //��ѧ����
    LOCAL_TYPE_SQGS,         //˯ǰ����
    LOCAL_TYPE_THGS,         //ͯ������
    LOCAL_TYPE_YYQM,         //Ӣ������
    LOCAL_TYPE_YYGS,         //Ӣ�����
    LOCAL_TYPE_YYEG,         //Ӣ�����
    LOCAL_TYPE_LXYY,         //��������
    
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

// ע�⣺�������Ҫ��YT_LOCAL_MODEL_TYPE�Ķ�Ӧһ��


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
