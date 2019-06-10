/** \file rda58xx.c
 * Functions for interfacing with the codec chip.
 */
#include "rda58xx.h"
#include "rda58xx_dbg.h"
#include "rda58xx_int_types.h"
#include "mbed_interface.h"

#define CHAR_CR             (0x0D)  // carriage return
#define CHAR_LF             (0x0A)  // line feed

#define MUPLAY_PROMPT       "AT+MUPLAY="
#define TX_DATA_PROMPT      "AT+MURAWDATA="
#define RUSTART_PROMPT      "AT+RUSTART="

#define MUSTOP_CMD          "AT+MUSTOP="
#define MUSTOP_CMD2         "AT+MUSTOP\r\n"
#define RUSTOP_CMD          "AT+RUSTOP\r\n"
#define VOLADD_CMD          "AT+CVOLADD\r\n"
#define VOLSUB_CMD          "AT+CVOLSUB\r\n"
#define SET_VOL_CMD         "AT+CVOLUME="
#define MUPAUSE_CMD         "AT+MUPAUSE\r\n"
#define MURESUME_CMD        "AT+MURESUME\r\n"
#define SET_MIC_GAIN_CMD    "AT+MUSETMICGAIN="
#define FT_CMD              "AT+FTCODEC="
#define GET_STATUS_CMD      "AT+MUGETSTATUS\r\n"
#define GET_VERSION_CMD     "AT+CVERSION\r\n"
#define UART_MODE_CMD       "AT+CMODE=UART_PLAY\r\n"
#define BT_MODE_CMD         "AT+CMODE=BT\r\n"
#define BT_BR_MODE_CMD      "AT+BTSWITCH=0\r\n"
#define BT_LE_MODE_CMD      "AT+BTSWITCH=1\r\n"
#define GET_BT_RSSI_CMD     "AT+BTGETRSSI\r\n"
#define LEGETCONFIGSTATE_CMD "AT+LEGETCONFIGSTATE\r\n"
#define LESWIFISCS_CMD      "AT+LESWIFISCS\r\n"
#define LEGETCONFIGINFO_CMD "AT+LEGETCONFIGINFO\r\n"
#define LESETINDICATION_CMD "AT+LESETINDICATION=\r\n"
#define LESETFEATURE_CMD    "AT+LESETFEATURE=\r\n"
#define LEGETINDSTATE_CMD   "AT+LEGETINDSTATE\r\n"

#define RX_VALID_RSP        "\r\nOK\r\n"
#define RX_VALID_RSP2       "OK\r\n"
#define AT_READY_RSP        "AT Ready\r\n"
#define AST_POWERON_RSP     "AST_POWERON\r\n"
#define CODEC_READY_RSP     "CODEC_READY\r\n"

//zt 20180702
#define SET_GPIO_MODE     	"AT+IOSETMODE="
#define SET_GPIO_DIR		"AT+IOSETDIR="
#define SET_GPIO_VAL		"AT+IOSETVALUE="
#define GET_GPIO_VAL		"AT+IOGETVALUE="

#define BT_PLAY_CMD				"AT+BTPLAY\r\n"
#define BT_PAUSE_CMD			"AT+BTPAUSE\r\n"
#define BT_FORWARD_CMD			"AT+BTFORWARD\r\n"
#define BT_BACKWARD_CMD			"AT+BTBACKWARD\r\n"
#define BT_VOLUP_CMD			"AT+BTVOLUP\r\n"
#define BT_VOLDOWN_CMD			"AT+BTVOLDOWN\r\n"
#define BT_GETAVRCPSTATUS_CMD	"AT+BTGETAVRCPSTATUS\r\n"

#define TIMEOUT_MS 5000

// #define RDA58XX_DEBUG

uint8_t tmpBuffer[64] = {0};
uint8_t tmpIdx = 0;
uint8_t res_str[32] = {0};

#define r_GPIO_DOUT (*((volatile uint32_t *)0x40001008))

//#define USE_TX_INT
int32_t atAckHandler(int32_t status);

void stringTohex(unsigned char *destValue, const char* src, int stringLen)
{
    int i = 0;
    int j = 0;

    for(; i<stringLen; i+=2, j++)
    {
       if(src[i] >= '0' && src[i] <= '9')
         destValue[j] = (src[i] - 0x30) << 4;
       else if(src[i] >= 'A' && src[i] <= 'F')
         destValue[j] = (src[i] - 0x37) << 4;
       else if(src[i] >= 'a' && src[i] <= 'f')
         destValue[j] = (src[i] - 0x57) << 4;

       if(src[1+i] >= '0' && src[1+i] <= '9')
         destValue[j] |= src[1+i]-0x30;
       else if(src[1+i] >= 'A' && src[1+i] <= 'F')
         destValue[j] |= src[1+i]-0x37;
       else if(src[1+i] >= 'a' && src[1+i] <= 'f')
         destValue[j] |= src[1+i]-0x57;
    }
}


void hexToString(char *destValue, const unsigned char* src, int hexLen)
{
    int i = 0;
    int j = 0;
    int temp;
    for(; i<hexLen; i++,j+=2)
    {
        temp = (src[i]&0xf0)>>4;
        if(temp >=0 && temp <=9)
        {
            destValue[j] = 0x30+temp;
        }
        else
        {
            destValue[j] = temp+0x37;
        }

        temp = (src[i]&0x0f);
        if(temp >=0 && temp <=9)
        {
            destValue[j+1] = 0x30+temp;
        }
        else
        {
            destValue[j+1] = temp+0x37;
        }
    }

}



/** Constructor of class RDA58XX. */
rda58xx::rda58xx(PinName TX, PinName RX, PinName HRESET):
    //_serial(TX, RX),
    _baud(3200000),
    _rxsem(0),
    _bufsem(0),
    _bufflag(false),
    _HRESET(HRESET),
    _mode(UART_MODE),
    _status(UNREADY),
    _rx_buffer_status(EMPTY),
    _rx_buffer_size(640/*64*/),
    _tx_buffer_size(0),
    _rx_idx(0),
    _tx_idx(0),
    _rx_buffer(NULL),
    _tx_buffer(NULL),
    _with_parameter(false),
    _at_status(NACK),
    _ready(false),
    _power_on(false),
    _ats(0),
    _atr(0)
{

	gpio_t gpioTX, gpioRX;

	gpio_init_out(&gpioTX, TX);
	gpio_init_out(&gpioRX, RX);
	gpio_write(&gpioTX, 0);
	gpio_write(&gpioRX, 0);

	wait_ms(10);

	_HRESET = 1;

	wait_ms(20);
	gpio_write(&gpioTX, 1);
	gpio_write(&gpioRX, 1);

	_serial = new RawSerial(TX, RX); 

    _serial->baud(_baud);
    _serial->attach(this, &rda58xx::rx_handler, RawSerial::RxIrq);
#if defined(USE_TX_INT)
    _serial->attach(this, &rda58xx::tx_handler, RawSerial::TxIrq);
    RDA_UART1->IER = RDA_UART1->IER & (~(1 << 1));//disable UART1 TX interrupt
    //RDA_UART1->FCR = 0x31;//set UART1 TX empty trigger FIFO 1/2 Full(0x21 1/4 FIFO)
#endif
    _HRESET = 1;
    _rx_buffer = new uint8_t [_rx_buffer_size * 2];
    _parameter.buffer = new uint8_t[64];
    _parameter.bufferSize = 0;
    atHandler = atAckHandler;
}

rda58xx::~rda58xx()
{
	if(_serial != NULL)
	{
		delete _serial;
		_serial = NULL;
	}	
    if (_rx_buffer != NULL)
        delete [] _rx_buffer;
    if (_parameter.buffer != NULL)
        delete [] _parameter.buffer;
}

void rda58xx::hardReset(void)
{
    _HRESET = 0;
    wait_ms(1);
    _mode = UART_MODE;
    _status = UNREADY;
    _rx_buffer_status = EMPTY;
    _tx_buffer_size = 0;
    _rx_idx = 0;
    _tx_idx = 0;
    _with_parameter = false;
    _at_status = NACK;
    _ready = false;
    _power_on = false;
    _ats = 0;
    _atr = 0;
    _HRESET = 1;
}

rda58xx_at_status rda58xx::bufferReq(mci_type_enum ftype, uint16_t size, uint16_t threshold)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: MUPLAY
    len = sprintf((char *)cmd_str, "%s%d,%d,%d\r\n", MUPLAY_PROMPT, ftype, size, threshold);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::stopPlay(void)
{
    // Send AT CMD: MUSTOP
    CODEC_LOG(INFO, MUSTOP_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)MUSTOP_CMD2);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::stopPlay(rda58xx_stop_type stype)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: MUSTOP
    len = sprintf((char *)cmd_str, "%s%d\r\n", MUSTOP_CMD, stype);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::pause(void)
{
    // Send AT CMD: MUPAUSE
    CODEC_LOG(INFO, MUPAUSE_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)MUPAUSE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::resume(void)
{
    // Send AT CMD: MURESUME
    CODEC_LOG(INFO, MURESUME_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)MURESUME_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::startRecord(mci_type_enum ftype, uint16_t size, uint16_t sample_rate)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: RUSTART
    len = sprintf((char *)cmd_str, "%s%d,%d,%d\r\n", RUSTART_PROMPT, ftype, size, sample_rate);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::stopRecord(void)
{
    // Send AT CMD: RUSTOP
    CODEC_LOG(INFO, RUSTOP_CMD);
    _at_status = NACK;
    _status = STOP_RECORDING;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)RUSTOP_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::setMicGain(uint8_t gain)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: SET_MIC_GAIN
    gain = (gain < 12) ? gain : 12;
	//gain = 9;
    len = sprintf((char *)cmd_str, "%s%d\r\n", SET_MIC_GAIN_CMD, gain);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::volAdd(void)
{
    // Send AT CMD: VOLADD
    CODEC_LOG(INFO, VOLADD_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)VOLADD_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::volSub(void)
{
    // Send AT CMD: VOLSUB
    CODEC_LOG(INFO, VOLSUB_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)VOLSUB_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::volSet(uint8_t vol)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: SET_VOL
    vol = (vol < 15) ? vol : 15;
    len = sprintf((char *)cmd_str, "%s%d\r\n", SET_VOL_CMD, vol);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

/** Send raw data to RDA58XX  */
rda58xx_at_status rda58xx::sendRawData(uint8_t *databuf, uint16_t n)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    if ((NULL == databuf) || (0 == n))
        return VACK;

    // Send AT CMD: MURAWDATA
    len = sprintf((char *)cmd_str, "%s%d\r\n", TX_DATA_PROMPT, n);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;
    if (VACK != _at_status) {
        CODEC_LOG(ERROR, "AT ACK=%d\n", _at_status);
        return _at_status;
    }

    // Raw Data bytes
    CODEC_LOG(INFO, "SendData\r\n");
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;

#if defined(USE_TX_INT)
    core_util_critical_section_enter();
    _tx_buffer = databuf;
    _tx_buffer_size = n;
    _tx_idx = 0;
    RDA_UART1->IER = RDA_UART1->IER | (1 << 1);//enable UART1 TX interrupt
#else
    core_util_critical_section_enter();

    while (n--) {
        _serial->putc(*databuf);
        databuf++;
    }
#endif
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

void rda58xx::setBaudrate(int32_t baud)
{
    _serial->baud(baud);
}

void rda58xx::setStatus(rda58xx_status status)
{
    _status = status;
}

rda58xx_status rda58xx::getStatus(void)
{
    return _status;
}

rda58xx_buffer_status rda58xx::getBufferStatus(void)
{
    _bufflag = true;
    _bufsem.wait();
    return _rx_buffer_status;
}

void rda58xx::clearBufferStatus(void)
{
    _rx_buffer_status = EMPTY;
}

uint8_t *rda58xx::getBufferAddr(void)
{
    return &_rx_buffer[_rx_buffer_size];
}

/* Set the RX Buffer size.
 * You CAN ONLY use it when you are sure there is NO transmission on RX!!!
 */
void rda58xx::setBufferSize(uint32_t size)
{
    _rx_buffer_size = size;
    delete [] _rx_buffer;
    _rx_buffer = new uint8_t [_rx_buffer_size * 2];
}

uint32_t rda58xx::getBufferSize(void)
{
    return _rx_buffer_size;
}

rda58xx_at_status rda58xx::factoryTest(rda58xx_ft_test mode)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: FT
    len = sprintf((char *)cmd_str, "%s%d\r\n", FT_CMD, mode);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::getCodecStatus(rda58xx_status *status)
{
    // Send AT CMD: GET_STATUS
    CODEC_LOG(INFO, GET_STATUS_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)GET_STATUS_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        if ((_parameter.buffer[0] >= '0') && (_parameter.buffer[0] <= '2')) {
            _parameter.status = (rda58xx_status)(_parameter.buffer[0] - 0x30);
        } else {
            _parameter.status = UNREADY;
        }
        CODEC_LOG(INFO, "status:%d\n", _parameter.status);
        _parameter.bufferSize = 0;
        *status = _parameter.status;
    }

    return _at_status;
}

rda58xx_at_status rda58xx::getChipVersion(char *version)
{
    CODEC_LOG(INFO, GET_VERSION_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)GET_VERSION_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.buffer[_parameter.bufferSize] = '\0';
        memcpy(&version[0], &_parameter.buffer[0], (_parameter.bufferSize + 1));
        _parameter.bufferSize = 0;
    }

    return _at_status;
}

rda58xx_at_status rda58xx::getBtRssi(int8_t *RSSI)
{
    // Send AT CMD: GET_BT_RSSI
    CODEC_LOG(INFO, GET_BT_RSSI_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)GET_BT_RSSI_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.value = 0;
        for (uint32_t i = 0; i < _parameter.bufferSize; i++) {
            if ((_parameter.buffer[i] >= '0') && (_parameter.buffer[i] <= '9')) {
                _parameter.value *= 10;
                _parameter.value += _parameter.buffer[i] - '0';
            }
        }
        _parameter.bufferSize = 0;
        *RSSI = (int8_t)(_parameter.value & 0xFF);
        CODEC_LOG(INFO, "BTRSSI:%d\n", *RSSI);
    }

    return _at_status;
}

/* Set mode, BT mode or Codec mode. In case of switching BT mode to Codec mode,
   set _status to MODE_SWITCHING and wait for "UART_READY" after receiving "OK" */
rda58xx_at_status rda58xx::setMode(rda58xx_mode mode)
{
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    if (UART_MODE == mode) {
        // Send AT CMD: UART_MODE
        CODEC_LOG(INFO, UART_MODE_CMD);
        _status = MODE_SWITCHING;
        _ready = false;
        core_util_critical_section_enter();
        _serial->puts((const char *)UART_MODE_CMD);
        _ats++;
        core_util_critical_section_exit();
    } else if (BT_MODE == mode) {
        // Send AT CMD: BT_MODE
        CODEC_LOG(INFO, BT_MODE_CMD);
        core_util_critical_section_enter();
        _serial->puts((const char *)BT_MODE_CMD);
        _ats++;
        core_util_critical_section_exit();
    }
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;
    if (VACK == _at_status) {
        _mode = mode;
        if (BT_MODE == mode)
            _ready = true;
    }

    return _at_status;
}

rda58xx_mode rda58xx::getMode(void)
{
    return _mode;
}

rda58xx_at_status rda58xx::setBtBrMode(void)
{
    // Send AT CMD: BT_BR_MODE
    CODEC_LOG(INFO, BT_BR_MODE_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)BT_BR_MODE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}


rda58xx_at_status rda58xx::SetName(const unsigned char* name)
{
    char cmd[60] ={0};

    sprintf(cmd, "AT+BTSETLNAME=%s\r\n", name);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}


rda58xx_at_status rda58xx::setBtLeMode(void)
{
    // Send AT CMD: BT_LE_MODE
    CODEC_LOG(INFO, BT_LE_MODE_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)BT_LE_MODE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::lesWifiScs(const unsigned char *adv)
{
    char cmd[120] = {0};
    // Send AT CMD: LESWIFISCS
    char advString[63] = {0};

    hexToString(advString, adv, 31);
    CODEC_LOG(INFO, LESWIFISCS_CMD);
    sprintf(cmd, "AT+LESWIFISCS=%s\r\n", advString);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::leGetConfigState(int8_t *configState)
{
    CODEC_LOG(INFO, LEGETCONFIGSTATE_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)LEGETCONFIGSTATE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.value = 0;
        for (uint32_t i =0; i < _parameter.bufferSize; i++) {
            if ((_parameter.buffer[i] >= '0') && (_parameter.buffer[i] <= '9')) {
                _parameter.value *= 10;
                _parameter.value += _parameter.buffer[i] - '0';
            }
        }
        _parameter.bufferSize = 0;
        *configState = (int8_t)(_parameter.value & 0xFF);
       CODEC_LOG(INFO, "LEGETCONFIGSTATE:%d\n", *configState);
    }

    return _at_status;
}

rda58xx_at_status rda58xx::leGetIndState(int8_t *indState)
{

    CODEC_LOG(INFO, LEGETINDSTATE_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)LEGETINDSTATE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.value = 0;
        for (uint32_t i =0; i < _parameter.bufferSize; i++) {
            if ((_parameter.buffer[i] >= '0') && (_parameter.buffer[i] <= '9')) {
                _parameter.value *= 10;
                _parameter.value += _parameter.buffer[i] - '0';
            }
        }
        _parameter.bufferSize = 0;
        *indState = (int8_t)(_parameter.value & 0xFF);
       CODEC_LOG(INFO, "LEGETINDSTATE:%d\n", *indState);
    }

    return _at_status;
}


rda58xx_at_status rda58xx::leGetConfigInfo(char *confinInfo)
{
    char value[256] = {0};
    CODEC_LOG(INFO, LEGETCONFIGINFO_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)LEGETCONFIGINFO_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.buffer[_parameter.bufferSize] = '\0';
        memcpy(value, &_parameter.buffer[0], (_parameter.bufferSize + 1));
        _parameter.bufferSize = 0;
    }

    stringTohex((unsigned char*)confinInfo, value, strlen(value));

    return _at_status;
}

rda58xx_at_status rda58xx::leClearConfigInfo(void)
{
    char cmd[60] ="AT+LECLEARCONFIGINFO\r\n";

    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}


rda58xx_at_status rda58xx::leSetIndication(const char* state)
{
    char cmd[60] ={0};
    char tmp[60] ={0};
    hexToString(tmp, (const unsigned char*)state, strlen(state));
    sprintf(cmd, "AT+LESETINDICATION=%s\r\n", tmp);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}


rda58xx_at_status rda58xx::SetAddr(const unsigned char* addr)
{
    char cmd[60] ={0};

    sprintf(cmd, "AT+BTSETADDR=%s\r\n", addr);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}




rda58xx_at_status rda58xx::leSetFeature(const char* features)
{
    char cmd[120] ={0};
    char temp[120] ={0};

    hexToString(temp, (const unsigned char*)features, strlen(features));

    sprintf(cmd, "AT+LESETFEATURE=%s\r\n", temp);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}


bool rda58xx::isReady(void)
{
    return _ready;
}

bool rda58xx::isPowerOn(void)
{
    return _power_on;
}

int32_t atAckHandler(int32_t status)
{
    if (VACK != (rda58xx_at_status) status) {
        switch ((rda58xx_at_status) status) {
        case NACK:
            CODEC_LOG(ERROR, "No ACK for AT command!\n");
            break;
        case IACK:
            CODEC_LOG(ERROR, "Invalid ACK for AT command!\n");
            break;
        default:
            CODEC_LOG(ERROR, "Unknown ACK for AT command!\n");
            break;
        }
        Thread::wait(osWaitForever);
    }

    return 0;
}

int32_t rda58xx::setAtHandler(int32_t (*handler)(int32_t status))
{
    if (NULL != handler) {
        atHandler = handler;
        return 0;
    } else {
        CODEC_LOG(ERROR, "Handler is NULL!\n");
        return -1;
    }
}

//zt 20180702
rda58xx_at_status rda58xx::setGPIOMode(int pin_name)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: MUPLAY
    len = sprintf((char *)cmd_str, "%s%d,0\r\n", SET_GPIO_MODE, pin_name);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);

	_at_status = NACK;
	_with_parameter = false;
	_rx_idx = 0;
	core_util_critical_section_enter();
	_serial->puts((const char *)cmd_str);
	_ats++;
	core_util_critical_section_exit();
	_rxsem.wait(TIMEOUT_MS);
	_atr = _ats;

	return _at_status;
}

rda58xx_at_status rda58xx::setGPIODir(int pin_name)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: MUPLAY
    len = sprintf((char *)cmd_str, "%s%d,0\r\n", SET_GPIO_DIR, pin_name);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);

	_at_status = NACK;
	_with_parameter = false;
	_rx_idx = 0;
	core_util_critical_section_enter();
	_serial->puts((const char *)cmd_str);
	_ats++;
	core_util_critical_section_exit();
	_rxsem.wait(TIMEOUT_MS);
	_atr = _ats;

	return _at_status;
}

rda58xx_at_status rda58xx::setGPIOVal(int pin_name, int val)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: MUPLAY
    len = sprintf((char *)cmd_str, "%s%d,%d\r\n", SET_GPIO_VAL, pin_name, val);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);

	_at_status = NACK;
	_with_parameter = false;
	_rx_idx = 0;
	core_util_critical_section_enter();
	_serial->puts((const char *)cmd_str);
	_ats++;
	core_util_critical_section_exit();
	_rxsem.wait(TIMEOUT_MS);
	_atr = _ats;

	return _at_status;
}

rda58xx_at_status rda58xx::getGPIOVal(char *res,int *size, int pin_name)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: MUPLAY
    len = sprintf((char *)cmd_str, "%s%d\r\n", GET_GPIO_VAL, pin_name);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
	_at_status = NACK;
	_with_parameter = true;
	_rx_idx = 0;
	core_util_critical_section_enter();
	_serial->puts((const char *)cmd_str);
	_ats++;
	core_util_critical_section_exit();
	_rxsem.wait(TIMEOUT_MS);
	_atr = _ats;

    if (VACK == _at_status) {
		*size = _parameter.bufferSize;
        _parameter.buffer[_parameter.bufferSize] = '\0';
        memcpy(&res[0], &_parameter.buffer[0], (_parameter.bufferSize + 1));
        _parameter.bufferSize = 0;
    }

	return _at_status;
}

rda58xx_at_status rda58xx::btPlay(void)
{
    // Send AT CMD: BT_LE_MODE
    CODEC_LOG(INFO, BT_PLAY_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)BT_PLAY_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::btPause(void)
{
    // Send AT CMD: BT_LE_MODE
    CODEC_LOG(INFO, BT_PAUSE_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)BT_PAUSE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::btVolup(void)
{
    // Send AT CMD: BT_LE_MODE
    CODEC_LOG(INFO, BT_VOLUP_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)BT_VOLUP_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::btVoldown(void)
{
    // Send AT CMD: BT_LE_MODE
    CODEC_LOG(INFO, BT_VOLDOWN_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)BT_VOLDOWN_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::btBackWard(void)
{
    // Send AT CMD: BT_LE_MODE
    CODEC_LOG(INFO, BT_BACKWARD_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)BT_BACKWARD_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::btForWard(void)
{
    // Send AT CMD: BT_LE_MODE
    CODEC_LOG(INFO, BT_FORWARD_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)BT_FORWARD_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::btGetA2dpStatus(app_a2dp_state_t *status)
{
    // Send AT CMD: GET_STATUS
    CODEC_LOG(INFO, BT_GETAVRCPSTATUS_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial->puts((const char *)BT_GETAVRCPSTATUS_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
		switch(_parameter.buffer[0])
		{
			case '0':
				*status = A2DP_IDLE;
				break;
			case '1':
				*status = A2DP_PLAY_AUDIO;
				break;
			case '2':
				*status = A2DP_CONNECTED;
				break;
			case '3':
				*status = A2DP_PLAY;
				break;
			default:
				*status = A2DP_IDLE;
				break;
		}
    }

    return _at_status;
}


// zafter 0628 end
void rda58xx::rx_handler(void)
{
    uint8_t count = (RDA_UART1->FSR >> 9) & 0x7F;

    count = (count <= (_rx_buffer_size - _rx_idx)) ? count : (_rx_buffer_size - _rx_idx);
    while (count--) {
        tmpBuffer[tmpIdx] = (uint8_t) (RDA_UART1->RBR & 0xFF);
        tmpIdx++;
    }

    memcpy(&_rx_buffer[_rx_idx], tmpBuffer, tmpIdx);
    _rx_idx += tmpIdx;

    switch (_status) {
    case RECORDING:
        if (_rx_buffer_size == _rx_idx) {
            memcpy(&_rx_buffer[_rx_buffer_size], &_rx_buffer[0], _rx_buffer_size);
            _rx_buffer_status = FULL;
            _rx_idx = 0;
            if (_bufflag) {
                _bufsem.release();
                _bufflag = false;
            }
        }
        break;

    case STOP_RECORDING:
#ifdef RDA58XX_DEBUG
        mbed_error_printf("%d\n", _rx_idx);
#endif
        if ((_rx_idx >= 4) && (_rx_buffer[_rx_idx - 1] == '\n')) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 4], 4);
            res_str[4] = '\0';
            if (res_str[0] && strstr(RX_VALID_RSP2, (char *)res_str) != NULL) {
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
                _rx_idx = 0;
                _status = PLAY;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
            }
        }
        if (_rx_buffer_size == _rx_idx) {
            memcpy(&_rx_buffer[0], &_rx_buffer[_rx_buffer_size - 10], 10);
            _rx_idx = 10;
        }
        break;

    case UNREADY:
        if ((_rx_idx >= 6) && ('\n' == _rx_buffer[_rx_idx - 1])) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 6], 6);
            res_str[6] = '\0';
            if (res_str[0] && strstr(CODEC_READY_RSP, (char *)res_str) != NULL) {
                _ready = true;
                _power_on = true;
                _rx_idx = 0;
                _status = STOP;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
            } else if (res_str[0] && strstr(AST_POWERON_RSP, (char *)res_str) != NULL) {
                _power_on = true;
                _rx_idx = 0;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
            } else if (res_str[0] && strstr(RX_VALID_RSP, (char *)res_str) != NULL) {
                if (_with_parameter) {
                    _parameter.bufferSize = ((_rx_idx - 10) < 64) ? (_rx_idx - 10) : 64;
                    memcpy(&_parameter.buffer[0], &_rx_buffer[2], _parameter.bufferSize);
                }
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
                _rx_idx = 0;
            } else {
#ifdef RDA58XX_DEBUG
#if 1
                _rx_buffer[_rx_idx] = '\0';
                mbed_error_printf("%s\n", _rx_buffer);
#else
                for (uint32_t i = 0; i < _rx_idx; i++) {
                    mbed_error_printf("%02X#", _rx_buffer[i]);
                }
#endif
#endif
            }
        }
        break;

    case MODE_SWITCHING:
        if ((_rx_idx >= 4) && ('\n' == _rx_buffer[_rx_idx - 1])) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 4], 4);
            res_str[4] = '\0';
            if (res_str[0] && strstr(RX_VALID_RSP2, (char *)res_str) != NULL) {
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
                _status = UNREADY;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
                _rx_idx = 0;
            } else {
#ifdef RDA58XX_DEBUG
#if 1
                _rx_buffer[_rx_idx] = '\0';
                mbed_error_printf("%s", _rx_buffer);
#else
                for (uint32_t i = 0; i < _rx_idx; i++) {
                    mbed_error_printf("%02X#", _rx_buffer[i]);
                }
#endif
#endif
                _at_status = IACK;
                _status = STOP;
            }
        }
        break;

    default:
        if ((_rx_idx >= 4) && ('\n' == _rx_buffer[_rx_idx - 1])) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 4], 4);
            res_str[4] = '\0';
            if (res_str[0] && strstr(RX_VALID_RSP2, (char *)res_str) != NULL) {
                if (_with_parameter) {
                    _parameter.bufferSize = ((_rx_idx - 10) < 64) ? (_rx_idx - 10) : 64;
                    memcpy(&_parameter.buffer[0], &_rx_buffer[2], _parameter.bufferSize);
                }
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
                _rx_idx = 0;
            } else {
#ifdef RDA58XX_DEBUG
#if 1
                _rx_buffer[_rx_idx] = '\0';
                // mbed_error_printf("%s", _rx_buffer);
#else
                for (uint32_t i = 0; i < _rx_idx; i++) {
                    mbed_error_printf("%02X#", _rx_buffer[i]);
                }
#endif
#endif
                _at_status = IACK;
            }
        }
        break;
    }

    tmpIdx = 0;

    if (_rx_idx >= _rx_buffer_size)
        _rx_idx = 0;
}

void rda58xx::tx_handler(void)
{
    if ((_tx_buffer != NULL) || (_tx_buffer_size != 0)) {
        uint8_t n = ((_tx_buffer_size - _tx_idx) < 64) ? (_tx_buffer_size - _tx_idx) : 64;
        while (n--) {
            RDA_UART1->THR = _tx_buffer[_tx_idx];
            _tx_idx++;
        }

        if (_tx_idx == _tx_buffer_size) {
            RDA_UART1->IER = (RDA_UART1->IER) & (~(0x01L << 1));//disable UART1 TX interrupt
            _tx_buffer = NULL;
            _tx_buffer_size = 0;
            _tx_idx = 0;
        }
    } else {
        RDA_UART1->IER = (RDA_UART1->IER) & (~(0x01L << 1));//disable UART1 TX interrupt
        _tx_buffer_size = 0;
        _tx_idx = 0;
        return;
    }
}

