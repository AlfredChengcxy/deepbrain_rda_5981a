#ifndef __AIRKISS_H__
#define __AIRKISS_H__

#include "mbed.h"
#include "rtos.h"
#include "WiFiStackInterface.h"

#define USE_THREAD 0

class Airkiss
{
public:
	static Airkiss & Instance();
	~Airkiss();
	class IOnEvent
	{
	public:
		virtual int on_airkiss_wait()=0;
		virtual int on_airkiss_connect()=0;
		virtual int on_airkiss_finish(char * strSSID,char * strPWD)=0;
	};	

	void SetEventListener(IOnEvent* _pEvent);
	void AirkissStart();
	void AirkissFinish();
	void Init(WiFiStackInterface * pWiFiStackInterface);
	bool IsAirkiss();
	void AirkissStop();
	int AirkissScanChannelex(rda5981_scan_result * pResult,int nLen);
private:
	Airkiss();
#if USE_THREAD		
	void Run();
#endif

private:
	static Airkiss mInstance;
#if USE_THREAD
	rtos::Thread thAirkiss;
#endif		
};



#endif /* AIRKISS_INTERFACE_H_ */
