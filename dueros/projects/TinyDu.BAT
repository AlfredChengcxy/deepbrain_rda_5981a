SET PATH=C:\Keil_v5\ARM\ARMCC\Bin;C:\yotta\python\;C:\yotta\python\Scripts;C:\Python27\;C:\Python27\Scripts;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Program Files (x86)\CMake\bin;C:\ninja;C:\Program Files (x86)\GNU Tools ARM Embedded\4.9 2015q3\bin;C:\Program Files (x86)\GNU Tools ARM Embedded\4.9 2015q3\bin;C:\ninja;C:\Keil_v5\ARM\ARMCC\bin;C:\Program Files (x86)\Windows Kits\8.1\Windows Performance Toolkit\;C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.2\arm\bin;C:\Program Files\TortoiseHg\;C:\Program Files\Git\bin\;C:\Windows\System32\
SET CPU_TYPE=MK64FN1M0xxx12
SET CPU_VENDOR=Freescale Semiconductor
SET UV2_TARGET=TinyDu
SET CPU_CLOCK=0x07270E00

python clean-obj.py

"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\hal_cm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_cmsis.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_event.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_list.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_mailbox.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_membox.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_memory.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_mutex.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_oseventobserver.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_robin.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_semaphore.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_system.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_task.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_time.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rt_timer.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rtx_conf_cm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\crc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_clock_config.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_phy.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_overrides.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\peripheralpins.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\hardware_init_mk64f12.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\k64f_emac.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\cmsis_nvic.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\entropy_hardware_poll.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pwmout_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\serial_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\spi_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\storage_driver.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\system_mk64f12.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\us_ticker.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\hal_cm4.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmAsm" --Via ".\build\uvision5\svc_table._ia"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmAsm" --Via ".\build\uvision5\startup_mk64f12._ia"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\sys.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\analogin_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\analogout_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\gpio_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\gpio_irq_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\i2c_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lp_ticker.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_api_lib.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_api_msg.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_err.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_netbuf.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_netdb.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_netifapi.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_sockets.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_tcpip.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pinmap.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\port_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rtc_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\sleep.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_checksum.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_memcpy.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_sys_arch.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\analogin.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\busin.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\businout.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\busout.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\callchain.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\can.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ethernet.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\filebase.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\filelike.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\filepath.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\filesystemlike.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\i2c.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\i2cslave.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\interruptin.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\interruptmanager.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\localfilesystem.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_alloc_wrappers.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_assert.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_board.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_critical.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_error.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_gpio.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_interface.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_lp_ticker_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_mem_trace.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_pinmap_common.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_rtc_time.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_semihost_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_ticker_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_us_ticker_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed_wait_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rawserial.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\retarget.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\serial.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\serialbase.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\spi.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\spislave.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\stream.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ticker.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\timeout.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\timer.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\timerevent.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_def.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_dhcp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_dns.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_init.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_mem.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_memp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_netif.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_pbuf.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_raw.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_stats.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_tcp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_tcp_in.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_tcp_out.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_timers.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_udp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_adc16.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_clock.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_cmp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_cmt.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_common.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_crc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_dac.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_dmamux.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_dspi.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_dspi_edma.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_edma.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_enet.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_ewm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_flash.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_flexbus.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_flexcan.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_ftm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_gpio.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_i2c.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_i2c_edma.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_llwu.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_lptmr.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_mpu.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_pdb.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_pit.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_pmc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_rcm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_rnga.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_rtc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_sai.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_sai_edma.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_sdhc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_sim.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_smc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_uart.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_uart_edma.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_vref.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\fsl_wdog.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\strategy.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\support_funcs.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_autoip.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_icmp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_igmp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_inet.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_inet_chksum.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_ip.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_ip_addr.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_ip_frag.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ethernetinterface.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_stack.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\main.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_etharp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_ethernetif.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_slipif.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\networkstack.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\nsapi_dns.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\socket.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\socketaddress.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\tcpserver.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\tcpsocket.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\udpsocket.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_auth.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_chap.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_chpms.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_fsm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_ipcp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_lcp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_magic.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_md5.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_pap.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_ppp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_ppp_oe.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_randm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_vj.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mutex.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rtos_idle.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rtostimer.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\semaphore.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\thread.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_asn1_dec.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_asn1_enc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_mib2.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_mib_structs.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_msg_in.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\lwip_msg_out.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\cfstore_fnmatch.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\cfstore_test.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\configuration_store.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\greentea_serial.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\test_env.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\unity.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\unity_handler.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\utest_case.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\utest_default_handlers.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\utest_greentea_handlers.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\utest_harness.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\utest_shim.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\utest_stack_trace.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\utest_types.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\aes.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\aesni.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\arc4.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\asn1parse.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\asn1write.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\base64.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\bignum.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\blowfish.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\camellia.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ccm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\certs.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\cipher.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\cipher_wrap.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ctr_drbg.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\debug.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\des.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\dhm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ecdh.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ecdsa.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ecjpake.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ecp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ecp_curves.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\entropy.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\entropy_poll.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\error.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\gcm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\havege.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\hmac_drbg.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\md.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\md2.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\md4.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\md5.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\md_wrap.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\memory_buffer_alloc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\net.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\oid.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\padlock.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pem.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pk.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pk_wrap.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pkcs11.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pkcs12.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pkcs5.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pkparse.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\pkwrite.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\platform.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ripemd160.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\rsa.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\sha1.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\sha256.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\sha512.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ssl_cache.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ssl_ciphersuites.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ssl_cli.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ssl_cookie.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ssl_srv.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ssl_ticket.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\ssl_tls.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\threading.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\timing.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\version.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\version_features.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\x509.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\x509_create.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\x509_crl.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\x509_crt.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\x509_csr.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\x509write_crt.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\x509write_csr.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\xtea.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\build\uvision5\mbed-utest-shim.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmLink" --Via ".\build\uvision5\TinyDu.lnp"
