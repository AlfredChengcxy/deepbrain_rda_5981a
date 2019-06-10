

#include "mbed.h"
#include "YTLocal.h"
#include "SDMMCFileSystem.h"
#include "FATDirHandle.h"
#include "audio.h"
#include "lightduer_log.h"

#include "baidu_media_manager.h"

extern SDMMCFileSystem sd;
PlayLocal PlayLocal::s_instance;
static bool s_is_initialed;
char currDirNameStr[100];



static const FlashMusicData g_ModelTipSound[] =
{
	YT_LOCAL_ERGE,sizeof(YT_LOCAL_ERGE),
	YT_LOCAL_GUSHI,sizeof(YT_LOCAL_GUSHI),
	YT_LOCAL_GUOXUE,sizeof(YT_LOCAL_GUOXUE),
	YT_LOCAL_YINGYU,sizeof(YT_LOCAL_YINGYU)
};

static const char* g_LocalPlayModelString[LOCAL_TYPE_END]={
#if 1
	"music",	 //儿歌
	"story",	 //故事
	"english",	 //英语
	"guoxue",	 //国学	
#else
    "儿歌",
    "故事",
    "国学",
    "唐诗",
    "英语",
    "收藏",
    "早教",
    "百科",
    "娱乐",
    "笑话",
    
    "儿歌精选",
    "唐诗精选",
    "国学经典",
    "睡前故事",
    "童话故事",
    "英语启蒙",
    "英语故事",
    "英语儿歌",
    "流行音乐",
    
    "music",     //儿歌
    "story",     //故事
    "guoxue",    //国学
    "tangshi",   //唐诗
    "english",   //英语
    "favorite",  //收藏
    "engmusic",  //故事


    "",          //根目录. END
#endif
};


static const FlashMusicInfo g_FlashMusic[] =
{
#if defined(___YT_LOCAL_PLAY_FLASH_SUPPORT__)
                                     
#endif
};



PlayLocal &PlayLocal::instance()
{
    return s_instance;
}

void PlayLocal::initVar()
{
    int i=0;

    //init play type define
    nPlayType = YT_LOCAL_PLAY_TF;
    
    nCurModelIndex = 0 ;
    CurIndex = -1 ;

    //init file count
    for(i=0;i<getMaxModelCount();i++)
    {
        nModelMaxFiles[i] = 0;
    }
}

PlayLocal::PlayLocal()
{
    initVar();
}

PlayLocal::~PlayLocal()
{
    //if(ftmp)
    //  FileClose();
}

void PlayLocal::reInit(YT_LOCAL_PLAY_TYPE type)
{

    initVar();
    
    nPlayType = type;
    
    init();
    
}

int PlayLocal::initTFFileInfo()
{
    int ret = 0 ;
    int i;
    for(i=0;i<getMaxModelCount();i++) //init every dir
    {
        nCurModelIndex = i;
        initDir();
        ret += nModelMaxFiles[nCurModelIndex];
    }
    return ret;
}

int PlayLocal::initFlashFileInfo()
{
    int ret = 0 ;
    int i;
    for(i=0;i<getMaxModelCount();i++) //init
    {
        nCurModelIndex = i;
        nModelMaxFiles[nCurModelIndex]= g_FlashMusic[nCurModelIndex].count;
        ret += nModelMaxFiles[nCurModelIndex];
    }
    return ret;
}


int PlayLocal::initSpiFlashFileInfo()
{
    int ret = 0 ;
    int i;
    for(i=0;i<getMaxModelCount();i++) //init
    {
        nCurModelIndex = i;
        nModelMaxFiles[nCurModelIndex]= g_FlashMusic[nCurModelIndex].count;
        ret += nModelMaxFiles[nCurModelIndex];
    }
    return ret;	
}

void PlayLocal::init()
{

    int initfileCount = 0 ;
   
    if(nPlayType == YT_LOCAL_PLAY_TF)
    {
        initfileCount = initTFFileInfo();
        if(initfileCount==0)
            nPlayType = YT_LOCAL_PLAY_FLASH; //auto switch to FLASH play;
    }

    if(nPlayType == YT_LOCAL_PLAY_FLASH)
    {
        initfileCount = initFlashFileInfo();
    }
    
    if(initfileCount<=0)
    {
        DUER_LOGI("not file play.[%d]",(int)nPlayType);
        s_is_initialed = false;
        return;
    }

    CurIndex = -1 ;
    nCurModelIndex = -1 ;
    bLoopAllAudio = true;

	s_is_initialed = true;
	DUER_LOGI("Local play init OK. [%d],[%d]",(int)nPlayType,initfileCount);

}

bool PlayLocal::checkTFCardDirExist()
{
    DirHandle *dir;
    int i;
    int oldIndex = nCurModelIndex;
    bool ret = true;
    int flag = 0;

    if(sd.disk_status() != 0) //ERROR
    {
        DUER_LOGI("not tf card !!!");
        return false;
    }
    
    for(i=0;i<getMaxModelCount();i++) //init every directory exist
    {
        nCurModelIndex = i;
        dir = opendir(getModelDirString());
        if(!dir)
            flag++;
        dir->closedir();
    }

    nCurModelIndex = oldIndex;
    
    if( flag == getMaxModelCount()-1) //all not exist
        flag = false;
        
    return ret;

}

void PlayLocal::initDir()
{
    DirHandle *dir;
    struct dirent *ptr;
    int fileCount = 0;

    if(nPlayType != YT_LOCAL_PLAY_TF)
    {
        DUER_LOGI("ERROR! play type not tf card.\n");
        return;
    }
    
    if(sd.disk_status() != 0) //ERROR
    {
        nModelMaxFiles[nCurModelIndex] = 0 ;
        return;
    }
    
    //ftmp = fopen(m_tmp, "w+");
    //if(NULL == ftmp) {
    //	LOG("Could not open file for write!\r\n");
    //	return;
    //}
    
    dir = opendir(getModelDirString());
    if(!dir)
    {
        nModelMaxFiles[nCurModelIndex] = 0 ;
        //s_is_initialed = false;
        return;
    }
    //char file_name[128] = {0};

    while((ptr = dir->readdir()) != NULL) 
    {
        DUER_LOGI("file_name:%s", ptr->d_name);		
        if(isMp3(ptr->d_name))
        {
            //if(ftmp)
            {
                //sprintf(file_name,"%s%s\r\n","/sd/music/",ptr->d_name);
                //fwrite(file_name, sizeof(file_name), 1, ftmp);			  
                fileCount++;
            }
        }
    }

    nModelMaxFiles[nCurModelIndex]=fileCount;
    //if(MaxFiles > 0)
    //  s_is_initialed = true;

    dir->closedir();

    //if(ftmp)
    //  fseek(ftmp, 0, SEEK_SET);	

    //if(ftmp)
    //  fclose(ftmp);
}

void PlayLocal::deinit()
{	
    s_is_initialed = false;
    //if(ftmp)
    //  fclose(ftmp);

    initVar();
}

bool PlayLocal::isMp3(char* name)
{
    bool ret = false;
    char* p1, *p2,*p3,*p4;
    char ext[6];
    
    if( name == NULL) return ret;
    if( name[0] == 0) return ret;

    p1 = name;
    p2 = p1 + strlen(name)-1;
    p4 = p2;
    
    memset(ext,0,6);
    p3 = ext + 5;
    while(p1 != p2)
    {
        if((p4 - p2) >= 6) break;
        if(*p2 == '.'){
            p3++;
            if( 0 == strncmp(p3,"mp3",3)) ret = true;
            break;
        } else {
            *p3 = *p2;
            if(*p3 >= 'A' && *p3 <= 'Z') *p3 += 0x20;
            p3--;
            p2--;
        }
    }
    return ret;
}


void PlayLocal::getFileByIndex(int index,char* path)
{
    if(nPlayType != YT_LOCAL_PLAY_TF)
    {
        DUER_LOGI("ERROR! play type not tf card.\n");
        return;
    }
        
    if(!s_is_initialed)
        return;

    if(sd.disk_check() != 0) //ERROR
    {
        //TF card error. switch flash play.
        DUER_LOGI("TF card error.\n");
        reInit(YT_LOCAL_PLAY_FLASH);
        //switchNext();
        return;
    }
    
    DirHandle *dir;
    struct dirent *ptr;
    int i = 0;

    dir = opendir(getModelDirString());
    if(!dir)
    {
        DUER_LOGI("can't open dir:%s", getModelDirString());
        return;
    }

    CurIndex = index;

    while((ptr = dir->readdir()) != NULL) 
    {
        if(isMp3(ptr->d_name))
        {
            if(i == CurIndex)
                break;
            i++;
        }
    }

    sprintf(path,"%s/%s\r\n",getModelDirString(),ptr->d_name);
    DUER_LOGI("play_file:%s", path);
    dir->closedir();
}

void PlayLocal::getPrevFilePath(char* path)
{
    if(nPlayType != YT_LOCAL_PLAY_TF)
    {
        DUER_LOGI("ERROR! play type not tf card.\n");
        return;
    }
        
    if(!s_is_initialed)
        return;

    if(nCurModelIndex < 0 || CurIndex <= 0)
    {
        if( bLoopAllAudio ) switchLocalPlayPrevModel();
        //CurIndex = MaxFiles - 1;
    }else{
        CurIndex--;		
		getFileByIndex(CurIndex, path);
    }
}

void PlayLocal::getNextFilePath(char* path)
{
    if(nPlayType != YT_LOCAL_PLAY_TF)
    {
        DUER_LOGI("ERROR! play type not tf card.\n");
        return;
    }
    
    if(!s_is_initialed)
        return;

    if(nCurModelIndex < 0 || CurIndex >= MaxFiles - 1)
    {
        if( bLoopAllAudio ) switchLocalPlayNextModel();
        //CurIndex = 0;
    }else{
        CurIndex++;		
		getFileByIndex(CurIndex, path);
    }
}

void PlayLocal::getPrevFileByFlashData(char **addr, unsigned int* len)
{
    if(nPlayType != YT_LOCAL_PLAY_FLASH)
    {
        DUER_LOGI("ERROR! play type not FLASH.\n");
        return;
    }
    
    if(!s_is_initialed)
        return;

	DUER_LOGI("nCurModelIndex = %d,CurIndex = %d\n",nCurModelIndex,CurIndex);

    if((nCurModelIndex < 0) || (CurIndex <= 0))
    {    	
        if(bLoopAllAudio) 
			switchLocalPlayPrevModel();
    }else{
        CurIndex--;		
		*addr = getLocalFlashPlayAddr();
		*len = getLocalFlashPlayLen();
    }
}


void PlayLocal::getNextFileByFlashData(char **addr, unsigned int* len)
{
    if(nPlayType != YT_LOCAL_PLAY_FLASH)
    {
        DUER_LOGI("ERROR! play type not FLASH.\n");
        return;
    }
    
    if(!s_is_initialed)
        return;

	DUER_LOGI("nCurModelIndex = %d,CurIndex = %d,MaxFiles = %d\n",nCurModelIndex,CurIndex,MaxFiles);

    if((nCurModelIndex < 0) || (CurIndex >= MaxFiles - 1))
    {
        if(bLoopAllAudio) 
			switchLocalPlayNextModel();
    }else{
        CurIndex++;		
		*addr = getLocalFlashPlayAddr();
		*len = getLocalFlashPlayLen();
    }
}


/*add*/
void PlayLocal::getNextAlbumData(char **addr, unsigned int* len)
{
    if(nPlayType != YT_LOCAL_PLAY_FLASH)
    {
        DUER_LOGI("ERROR! play type not FLASH.\n");
        return;
    }
    
    if(!s_is_initialed)
        return;

	DUER_LOGI("nCurModelIndex = %d,CurIndex = %d,MaxFiles = %d\n",nCurModelIndex,CurIndex,MaxFiles);
	

    if((nCurModelIndex < 0) || (CurIndex >= MaxFiles - 1))
    {
        if(bLoopAllAudio) switchLocalPlayNextModel();
    }
	else{	
		CurIndex = -1;

        nCurModelIndex++;
		nCurModelIndex %= getMaxModelCount() +1;
		*addr = (char*)g_ModelTipSound[nCurModelIndex].addr;
		*len = g_ModelTipSound[nCurModelIndex].len;	
    }

}

void PlayLocal::getPreAlbumData(char **addr, unsigned int* len)
{
    if(nPlayType != YT_LOCAL_PLAY_FLASH)
    {
        DUER_LOGI("ERROR! play type not FLASH.\n");
        return;
    }
    
    if(!s_is_initialed)
        return;

	DUER_LOGI("nCurModelIndex = %d,CurIndex = %d,MaxFiles = %d\n",nCurModelIndex,CurIndex,MaxFiles);

    if((nCurModelIndex < 0) || (CurIndex <= 0))
    {    	
        if(bLoopAllAudio) switchLocalPlayPrevModel();
    }
	else{
		CurIndex = -1;
		
        nCurModelIndex--;
		if(nCurModelIndex < 0)nCurModelIndex = getMaxModelCount();		
		nCurModelIndex %= getMaxModelCount()+1;
		
		*addr = (char*)g_ModelTipSound[nCurModelIndex].addr;
		*len = g_ModelTipSound[nCurModelIndex].len;
    }

}

void PlayLocal::playModelTipSound()
{	
	/*YTMediaManager::instance().play_data(g_ModelTipSound[nCurModelIndex].addr
		,g_ModelTipSound[nCurModelIndex].len
		,MEDIA_FLAG_LOCAL_ROUND);*/

	duer::MediaManager::instance().play_data(g_ModelTipSound[nCurModelIndex].addr,g_ModelTipSound[nCurModelIndex].len,duer::MEDIA_FLAG_LOCAL_MODE);
}

void PlayLocal::switchLocalPlayModel(YT_LOCAL_MODEL_TYPE type)
{
    if(type>=getMaxModelCount()){
        DUER_LOGI("type error!");
        return;
    }

     nCurModelIndex = type;
     CurIndex = -1 ; // init to:0 or -1 ?
     MaxFiles = nModelMaxFiles[nCurModelIndex];
}

void PlayLocal::switchLocalPlayPrevModel()
{
    int i = 0 ;
    int flag = 0 ;
    int max = getMaxModelCount();

    for(i = 0; i<max; i++)
    {
	    if(nCurModelIndex <= 0)
    	{
       		nCurModelIndex = max - 1;
    	}else{
        	nCurModelIndex--;
    	}	
	
        if( nModelMaxFiles[nCurModelIndex] != 0)
        {
            CurIndex = -1 ; // init to:0 or -1 ?
            MaxFiles = nModelMaxFiles[nCurModelIndex];
            flag = 1;
            break;
        }
    }

    if( flag == 1)
    {
        //find.
        //play tip sound ?????
        playModelTipSound();
        DUER_LOGI("switch to:%s\r\n",getModelDirString());
    }
	else
	{
        DUER_LOGI("switchLocalPlayPrevModel Error!!\r\n");
	}
}

void PlayLocal::switchLocalPlayNextModel()
{
    int i = 0 ;
    int flag = 0 ;
    int max = getMaxModelCount();

	for(i = 0; i < max; i++)
	{
	    if(nCurModelIndex >= max - 1)
	    {
	        nCurModelIndex = 0;
	    }else{
	        nCurModelIndex++;
	    }
		
        if( nModelMaxFiles[nCurModelIndex] != 0)
        {
            CurIndex = -1 ; // init to:0 or -1 ?
            MaxFiles = nModelMaxFiles[nCurModelIndex];
            flag = 1;
            break;
        }
	}

    if( flag == 1)
    {
        //find.
        //play tip sound ?????
        playModelTipSound();
        DUER_LOGI("switch to:%s\r\n",getModelDirString());
    }	
	else
	{
        DUER_LOGI("switchLocalPlayNextModel Error!!\r\n");
	}
}


YT_LOCAL_PLAY_TYPE PlayLocal::getLocalPlayType()
{
    return nPlayType;
}

void PlayLocal::setLocalPlayType(YT_LOCAL_PLAY_TYPE type)
{
    nPlayType = type;
}

char* PlayLocal::getLocalFlashPlayAddr()
{
    if(nPlayType != YT_LOCAL_PLAY_FLASH)
    {
        DUER_LOGI("getLocalFlashPlayAddr ERROR! play type not FLASH.\n");
        return 0;
    }
    
    return (char*)g_FlashMusic[nCurModelIndex].music_data[CurIndex].addr;
}

unsigned int PlayLocal::getLocalFlashPlayLen()
{
    if(nPlayType != YT_LOCAL_PLAY_FLASH)
    {
        DUER_LOGI("getLocalFlashPlayAddr ERROR! play type not FLASH.\n");
        return 0;
    }
    
    return g_FlashMusic[nCurModelIndex].music_data[CurIndex].len;
}

char* PlayLocal::getModelString()
{
    return (char*)g_LocalPlayModelString[nCurModelIndex];
}

char* PlayLocal::getModelDirString()
{
    char* p = getModelString();
    memset(currDirNameStr,0,100);
    if( p[0]== 0 ) strcpy(currDirNameStr,"/sd");
    else sprintf(currDirNameStr,"/sd/%s",p);
    return (char*)currDirNameStr;
}

int PlayLocal::getMaxModelCount()
{
    return nPlayType == YT_LOCAL_PLAY_TF ? LOCAL_TYPE_END : sizeof(g_FlashMusic)/sizeof(FlashMusicInfo);
}

int PlayLocal::getCurModelIndex()
{
    return nCurModelIndex;
}

void PlayLocal::setLoopAllAudio(bool bLoop)
{
    bLoopAllAudio = bLoop;
}

bool PlayLocal::getLoopAllAudio()
{
    return bLoopAllAudio;
}




