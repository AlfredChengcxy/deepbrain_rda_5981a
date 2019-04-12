#ifndef __CHANGE_VOICE_H__
#define __CHANGE_VOICE_H__

namespace duer{

void yt_voice_set_wave_head(char *wavHeader);
int yt_voice_change_init(char *data, int size);
int yt_voice_change_get_data(char **data, int *size);
void yt_voice_change_free();

}
#endif
