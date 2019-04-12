#ifndef VAD_CONFIG_H_
#define VAD_CONFIG_H_

enum VADTYPE {
    HEADER = 1
};

class VadConfiger {
public:
    VadConfiger();
    ~VadConfiger();

    bool load();

private:
    VADTYPE _vadType;
public:
    int _winSize;
    float speech_st_threshold;
    float speech_et_threshold;
};

#endif
