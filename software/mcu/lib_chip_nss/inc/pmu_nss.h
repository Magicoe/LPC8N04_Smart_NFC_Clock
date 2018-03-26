/*
 * Copyright (c), NXP Semiconductors
 * (C)NXP B.V. 2014-2017
 * All rights are reserved. Reproduction in whole or in part is prohibited without
 * the written consent of the copyright owner. NXP reserves the right to make
 * changes without notice at any time. NXP makes no warranty, expressed, implied or
 * statutory, including but not limited to any implied warranty of merchantability
 * or fitness for any particular purpose, or that the use will not infringe any
 * third party patent, copyright or trademark. NXP must not be liable for any loss
 * or damage arising from its use.
 */


#ifndef __PMU_NSS_H_
#define __PMU_NSS_H_

/** @defgroup PMU_NSS pmu: Power Management Unit driver
 * @ingroup DRV_NSS
 * The Power Management Unit driver (PMU) provides the API to control all the functionalities handled by the PMU HW
 * block. The PMU driver allows controlling the:
 *  -# Power modes
 *  -# Power switches
 *  -# Brown-Out detector
 *  -# NFC power detector
 *  -# Wakeup pin functionality
 *  -# RTC block clock source
 *  .
 *
 * @anchor pmu_powerarch_anchor
 * @par Power Architecture:
 *  The chip has 4 separate major power domains:
 *      - NFC (VNFC)
 *      - Always-On (VDD_ALON)
 *      - 1.2V Internal (VDD1V2)
 *      - 1.6V Internal (VDD1V6)
 *      .
 *  Power may be supplied via two possible sources:
 *      - External power supply pin (VDDBAT)
 *      - Built-in RFID rectifier (part of the VNFC power domain)
 *      .
 *  The VDD_ALON power domain is directly powered by one of these two power sources. The PMU, which is in the VDD_ALON
 *  power domain, is responsible for controlling the power for the 1V2 and 1V6 internal power domains.
 *
 * @par
 *  Both the VDDBAT pin and the VNFC power sources are connected to the VDD_ALON power domain by means of automatic
 *  switches. The switches define which power source is providing power to the VDD_ALON power domain and, consequently,
 *  the rest of the chip. The switches are, thus, never closed simultaneously.
 *
 * @par
 *  Even when power in the VDDBAT pin is present (when voltage is lower than 1.72V, power is not considered to be present),
 *  the respective switch will only be automatically closed when the RESETN pin is deasserted (an edge from low to high
 *  is detected) or at the time power is detected at the VNFC power domain (an edge from no power detected to power
 *  detected coming from the built-in RFID rectifier). In other words, even when power in the VDDBAT pin is present, the
 *  chip is only effectively powered up when one of these two triggers is detected. This switch can be manually set
 *  back to its initial open state using the #Chip_PMU_Switch_OpenVDDBat function.
 *
 * @par
 *  When power in the VNFC domain becomes present (when voltage is lower than 1.72V, power is not considered to be
 *  present), there are three possible scenarios:
 *      - No power is available in the VDDBAT pin
 *      - Power is also available in the VDDBAT pin, but the switch is still open
 *      - Power is also available in the VDDBAT pin, but the switch is already closed
 *      .
 *  In the first scenario, the VNFC switch is automatically closed and the chip will be powered via VNFC. @n
 *  In the second scenario, the presence of VNFC will trigger the VDDBAT switch to close. In the presence of the two power
 *  sources, the automatic switching mechanism will give priority to VDDBAT. @n
 *  In the last scenario, as the switch is already closed, the IC is already being powered by the VDDBAT pin. Hence, when
 *  VNFC becomes also present, priority will continue to be given to VDDBAT. @n
 *  As described above, it is always possible to manually send back the VDDBAT switch to its initial open state using
 *  the #Chip_PMU_Switch_OpenVDDBat function. This can be used to manually trigger a switchover from VDDBAT to VNFC
 *  power source. So it is possible to force the IC to be powered by VNFC, however this has to be done each time power
 *  in the VNFC domain becomes present.
 *
 * @par
 *  Switchover from one source to the other is automatically done in both directions without glitches in the VDD_ALON
 *  power domain.
 *
 * @par
 *  The VDD_ALON power domain is hard connected to the external bondpad ring (pins), hence, it is possible to provide
 *  power to the pins from both VDDBAT or VNFC. Note that in the latter case, the VDDBAT pin will be disconnected from
 *  the bondpad ring as it is only meant to be a power source itself.
 *
 * @par Power Modes:
 *  <dl><dt> Active: </dt><dd>
 *      This is the default power state after Power-On-Reset. In this state, all internal power domains (1V2 and 1V6) are
 *      powered. Thus, the ARM core and memories are clocked by the system clock, and peripherals are clocked (if enabled) 
 *      by the system clock or a dedicated peripheral clock. </dd>
 *  <dt> Sleep: </dt><dd>
 *      The only difference from Active to Sleep is that the clocking of the ARM core is stopped and program execution
 *      is suspended until an interrupt reaches the core or a reset condition is detected. All peripherals, if in use,
 *      continue operation, the processor state and registers as well as SRAM content are retained and the logic levels
 *      of pins remain static. Use the #Chip_PMU_PowerMode_EnterSleep function to enter the Sleep power mode after the
 *      wake-up interrupt sources are correctly configured. </dd>
 *  <dt> Deep Sleep: </dt><dd>
 *      In this power mode, as in Sleep mode, the clocking of the ARM core is also stopped . Moreover, all analog peripherals
 *      and Flash memory are powered down. However, as the SFRO keeps running, it is possible to leave the Watchdog timer
 *      or the general-purpose timers running (if required for timer controlled wake-up). The processor state and registers
 *      as well as SRAM content are retained and the logic levels of pins remain static. Use the #Chip_PMU_PowerMode_EnterDeepSleep
 *      function to enter the Deep Sleep power mode after the wake-up interrupt source are correctly configured
 *      and all the peripheral clocks, except the ones required, are disabled.@n
 *      The chip can exit the Deep Sleep power mode and resume execution on any Start Logic or Watchdog interrupt or restart
 *      execution in Active mode on a reset condition. </dd>
 *  <dt> Deep Power Down: </dt><dd>
 *      In this power mode, all the internal power domains (1V2 and 1V6) are switched off (SFRO is stopped).
 *      In other words, everything is powered down except the VDD_ALON power domain (PMU and RTC are still powered). This
 *      means that, besides the program execution and peripherals being stopped and the ARM core state being lost, all
 *      SRAM and register content is also lost (except registers residing in the PMU and RTC). All digital functional pins
 *      are tri-stated except for the WAKEUP (if previously configured), and RESETN pins. Use the
 *      #Chip_PMU_PowerMode_EnterDeepPowerDown function to enter the Deep Power Down power mode after the wake-up sources
 *      are correctly configured.@n
 *      The chip can exit the Deep Power Down power mode on a reset condition, WAKEUP pin asserted (active low), RTC wake-up
 *      counter expire event or NFC powered event (NVIC or ARM core not involved). After waking up from Deep Power
 *      Down mode, the code execution is restarted (never resumed). If the chip exits Deep Power Down mode due to a reset
 *      condition, all the registers from the PMU and RTC are also set to their default value, including RTC counters,
 *      PMU general purpose registers and status flags. @n
 *      Using the #Chip_PMU_PowerMode_GetDPDWakeupReason function it is possible to know if the chip woke up from Deep
 *      Power Down as well as the reason for the wake up. @n
 *      Note that, if there is a debugging session ongoing when you enter Deep Power Down mode, it will terminate and
 *      cannot be started again as long as the IC is in Deep Power Down mode. </dd>
 *  @anchor pmu_offmode_anchor
 *  <dt> Off: </dt><dd>
 *      In this "Off" mode, it is considered that power is present in the VDDBAT pin. In this mode everything in the IC
 *      is powered off except the HW logic that detects the triggers that cause the VDDBAT switch to close and consequently
 *      power the rest of the IC (see @ref pmu_powerarch_anchor "Power Architecture"). @n
 *      Use the #Chip_PMU_Switch_OpenVDDBat function to enter the "Off" mode. When the "Off" mode is left (due to RESETN
 *      asserted or power present in VNFC), the IC will start-up from scratch (POR). </dd></dl>
 *  Regardless of the power mode ("Off" is not included), RTC and PMU register values will be kept and functionality
 *  ensured until a full system reset occurs which sets all registers to their default state. A normal exit from Deep
 *  Power Down mode does not trigger a full system reset.
 *  @anchor pmu_dpdwarning_anchor
 *  @warning Once in Deep Power Down mode, it is no longer possible to start a debug session, hence, special care
 *      must be taken when calling #Chip_PMU_PowerMode_EnterDeepPowerDown with short "break-in" time between boot and
 *      the function call. In case Debug build is used, it is possible to introduce a 500ms delay by approaching an
 *      NFC field before entering Deep Power Down. This functionality is embedded in the #Chip_PMU_PowerMode_EnterDeepPowerDown
 *      function and allows starting a debug session in ICs that would otherwise never accept a debug connection anymore.
 *      See @ref NSS_DEBUG_CONSIDERATIONS for more details.
 *
 * @anchor pmu_power_source_switches_par
 * @par Power source switches:
 *  Each power source (VDDBAT pin and VNFC power domain) has a dedicated switch that connects it to the VDD_ALON power
 *  domain. These switches behave automatically, and give priority to the VDDBAT pin, as described above.@n
 *  The state of the switches can be individually checked using the #Chip_PMU_Switch_GetVNFC and the
 *  #Chip_PMU_Switch_GetVDDBat functions.
 *  This automatic switching mechanism can be disabled before entering Deep Power Down (See #Chip_PMU_PowerMode_EnterDeepPowerDown).
 *  This is typically done to keep power consumption to a minimum, however, special care must be taken when entering
 *  Deep Power Down with the power source switches disabled when running from an almost empty battery (VDDBat ~ 1.74V).
 *  The consequences are that if the battery drains out (VDDBat < 1.74V) during Deep Power Down and the automatic switching
 *  mechanism is disabled, the IC won't be able to switch to VNFC power and start execution using the energy of an NFC
 *  field only. @n
 *  The proposed solution is to use the Brown Out detector feature of the IC (See @ref pmu_brown_out_detector_par
 *  "Brown-Out-Detector") to find out if VDDBat is below 1.8V to decide whether or not it is safe to disable the automatic
 *  switching mechanism.
 *
 * @anchor pmu_brown_out_detector_par
 * @par Brown-Out-Detector:
 *  The PMU provides the possibility to monitor the power source voltage and detect whether it drops to 1.8V or below.
 *  This functionality is disabled by default and can be enabled using the #Chip_PMU_SetBODEnabled function. The detection
 *  of a Brown-Out condition is combined with interrupt generation. The current state of the BOD can be checked with the
 *  #Chip_PMU_GetStatus function and the interrupt generation can be enabled with the #Chip_PMU_Int_SetEnabledMask function.
 *  If enabled, the BOD functionality will be working during all power modes, although its interrupt can only be used as
 *  a wake-up source in Sleep mode. The occurrence of a BO condition can, thus, only be known after wakeup from Sleep,
 *  Deep Sleep or Deep Power Down.@n
 *  Note that the #Chip_PMU_GetStatus function will not provide the BOD interrupt flag that retains its value after
 *  triggered, but only the current status of the BOD (if the brown out condition is not present anymore, the current
 *  status will be cleared). To get the BOD interrupt flag, use the #Chip_PMU_Int_GetRawStatus function.
 *
 * @par NFC power detector:
 *  The PMU provides the possibility to detect, in SW, the presence of power in the VNFC power domain. This functionality
 *  is always enabled and the current status of the power in the VNFC power domain can be checked using the #Chip_PMU_GetStatus
 *  function. The respective interrupt generation can be enabled with the #Chip_PMU_Int_SetEnabledMask function.@n
 *  The NFC power detector functionality will be working during all power modes, although its interrupt can only be used as
 *  a wake-up source in Sleep mode (via triggering of corresponding interrupt in the NVIC). To wake-up from Deep Sleep with
 *  an NFC field, you can use the respective start logic interrupt (see @ref startLogic_anchor "System wake-up start logic"). @n
 *  Note that the #Chip_PMU_GetStatus function will not provide the VNFC power interrupt flag that retains its value
 *  after triggered, but only the current status of the VNFC power (if the VNFC power is not present anymore, the current
 *  status will be cleared). To get the VNFC power interrupt flag, use the #Chip_PMU_Int_GetRawStatus function.
 *  @note It is not possible to reliably detect whether the power-off mode has been left due to power present in VNFC.
 *      As mentioned, you can get the current status using #Chip_PMU_GetStatus but this will not tell you whether the
 *      VDDBAT pin got connected and the IC started up due to the presence of an NFC field: it is possible that the
 *      NFC field triggers VDDBAT pin to be connected but is already gone by the time #Chip_PMU_GetStatus is called; or
 *      that the field was only present after VDDBAT pin got connected (due to a different reason) but just in time for
 *      the #Chip_PMU_GetStatus call.
 *
 * @par Wakeup Pin:
 *  The PMU provides the possibility to wakeup from Deep Power Down mode when a high-to-low transition in the PIO0_0 pin
 *  is detected. By default, after a reset, this functionality is disabled, meaning it is not possible to exit the
 *  Deep Power Down mode via the PIO0_o pin. To enable this functionality, the application must:
 *  - Call #Chip_PMU_SetWakeupPinEnabled with @c true as argument, and
 *  - Configure the PIO0_0 pin for input with the pull-up resistor enabled.
 *
 * @par RTC block clock source:
 *  The timer oscillator (TFRO) is the default clock source for the PMU and RTC. The PMU block is always clocked by the
 *  TFRO, however, for the RTC block it is possible to select its clock source (only between TFRO or NONE).
 *  This can be done using the #Chip_PMU_SetRTCClockSource function, although TFRO is selected by default as clock source.
 *
 * @par General Purpose data registers:
 *  The PMU can hold 5 words of data in its own registers which is retained during Deep Power Down mode. This data will
 *  only be lost after a Power-On-Reset or an external reset (RESETN pin asserted). This data region can be accessed using
 *  the #Chip_PMU_SetRetainedData and #Chip_PMU_GetRetainedData functions. This data section is available to be used by 
 *  the application layer as a simple data container to store data that needs to survive a Deep Power Down.
 *
 * @anchor pmu_syncwait_warning_anchor
 * @warning Each PMU block register read and write performs at least one "wait" period that may take up to 100us to
 *  complete due to hardware synchronization within the module. All the functions in this driver make at least one such
 *  register access unless stated otherwise. Impact on performance should be carefully considered especially when
 *  calling PMU driver functions in ISRs context and high-priority threads. If a PMU block register read/write preempts
 *  another PMU block register read/write, the procedure is repeated leading to another "wait" period, until no
 *  preemption is detected.@n
 *
 * @par Example 1 - Perform low-power temperature measurement:
 *  Requirements:
 *      - Use the lowest power mode possible (perform the measurement in Sleep mode and wake up when measurement is 
 *          finished)
 *      - resolution: 12 bits
 *      .
 *  main code:
 *  @snippet pmu_nss_example_1.c pmu_nss_example_1
 *  handle interrupt code:
 *  @snippet pmu_nss_example_1.c pmu_nss_example_1_irq
 *
 * @par Example 2 - Increment a counter in SRAM every time a pin is pulled high:
 *  Requirements:
 *      - Increment a counter in SRAM at every PIO0_1 rising edge
 *      - Use the lowest power mode possible (take Deep Sleep as the default state and use Start Logic to wake up)
 *      .
 *  main code:
 *  @snippet pmu_nss_example_2.c pmu_nss_example_2
 *  handle interrupt code:
 *
 * @par Example 3 - Increment separate counters in PMU General Purpose Registers on different actions:
 *  Requirements:
 *      - Increment a counter in PMU General Purpose Registers every time the WAKEUP pin is asserted from high to low
 *      - Increment a counter in PMU General Purpose Registers every time the VNFC power presence is noticed
 *      - Use the lowest power mode possible (take Deep Power Down mode as the default state)
 *      - The counter is not incremented when the pin is asserted from high to low when the chip is already in Active mode
 *      .
 *  main code:
 *  @snippet pmu_nss_example_3.c pmu_nss_example_3
 *
 * @par Example 4 - VNFC as trigger to start application:
 *  Requirements:
 *      - Battery connected to VDDBAT pin
 *      - Application shall not start if external reset pin is asserted
 *      - Application shall start when VNFC is present for the first time
 *      .
 *  code:
 *  @snippet pmu_nss_example_4.c pmu_nss_example_4
 * @{
 */

/** NSS Power Management Unit register block structure */
typedef struct NSS_PMU_S {
    __IO uint32_t PCON; /*!< Power control Register */
    __IO uint32_t GPREG[5]; /*!< General purpose Registers 0..4 */
    __I uint32_t PSTAT; /*!< Power Management Unit status register */
    __I uint32_t RESERVED1; /* next field at offset 0x020 */
    __I uint32_t ACCSTAT; /*!< Access status register */
    __IO uint32_t LDO1V6; /*!< Analog 1.6V LDO trimming */
    __I uint32_t RESERVED2; /* next field at offset 0x02C */
    __IO uint32_t TMRCLKCTRL; /*!< Timer clock control register */
    __IO uint32_t IMSC; /*!< Interrupt mask set and clear register */
    __I uint32_t RIS; /*!< Raw interrupt status register */
    __I uint32_t MIS; /*!< Masked interrupt status register */
    __O uint32_t ICR; /*!< Interrupt clear register */
} NSS_PMU_T;

/** Possible reasons for waking up from Deep Power Down */
typedef enum PMU_DPD_WAKEUPREASON {
    PMU_DPD_WAKEUPREASON_RTC = 1, /*!< RTC wakeup-downcounter expired */
    PMU_DPD_WAKEUPREASON_NFCPOWER = 2, /*!< Power detected in the VNFC domain */
    PMU_DPD_WAKEUPREASON_WAKEUPPIN = 3, /*!< WAKEUP pin negative edge detected */
    PMU_DPD_WAKEUPREASON_NONE = 0xFF /*!< IC hasen't woken-up from Deep Power Down since last POR event, RESETN pin assertion or
     Sleep/DeepSleep mode entry */
} PMU_DPD_WAKEUPREASON_T;

/** Possible clock sources for the RTC block */
typedef enum PMU_RTC_CLOCKSOURCE {
    PMU_RTC_CLOCKSOURCE_NONE = 0, /*!< None of the clock sources is selected for the RTC block */
    PMU_RTC_CLOCKSOURCE_TFRO = 1, /*!< Represents the TFRO as clock source for the RTC block */
} PMU_RTC_CLOCKSOURCE_T;

/** Possible PMU interrupt flags */
typedef enum PMU_INT {
    PMU_INT_BROWNOUT = (1 << 0), /*!< Interrupt that is triggered when a Brown-Out is detected */
    PMU_INT_NFCPOWER = (1 << 1), /*!< Interrupt that is triggered when power in the VNFC domain is detected */
    PMU_INT_NONE = 0, /*!< Disable all Interrupts */
    PMU_INT_ALL = 0x3 /*!< Enable all Interrupts */
} PMU_INT_T;

/** Possible status conditions of the PMU block (simultaneous bits are possible) */
typedef enum PMU_STATUS {
    PMU_STATUS_BROWNOUT = (1 << 5), /*!< Indicates whether a Brown-Out condition is currently being detected */
    PMU_STATUS_VDD_NFC = (1 << 7), /*!< Indicates whether power in the VNFC domain is currently being detected */
} PMU_STATUS_T;

/**
 * Sets the chip into Sleep power mode
 * @note The program execution will be blocked inside this function for as long as the chip is in Sleep mode.
 *  After the chip exits Sleep mode and the respective ISR is executed, the execution will be resumed and this function
 *  will return.
 * @note The observer flags that indicate which power modes were entered are cleared before entering this standby mode.
 *  Refer to @b Power @b Modes section for more details on the Sleep mode.
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_PowerMode_EnterSleep(void);

/**
 * Sets the chip into Deep Sleep power mode
 * @note The program execution will be blocked inside this function for as long as the chip is in Deep Sleep mode.
 *  After the chip exits Deep Sleep mode and the respective ISR is executed, the execution will be resumed and this
 *  function will return.
 * @note All peripheral clocks must be disabled beforehand except for the Timers and Watchdog, if required. The power
 *  state after wakeup of some blocks must also be configured beforehand. Please refer to @ref SYSCON_NSS "SysCon driver"
 *   documentation.
 * @note The observer flags that indicate which power modes were entered are cleared before entering this standby mode.
 *  Refer to @b Power @b Modes section for more details on the Deep Sleep mode.
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_PowerMode_EnterDeepSleep(void);

/**
 * Sets the chip into Deep Power Down power mode
 * @param enableSwitching : Enables/Disables the automatic switching mechanism before entering Deep Power Down.
 * @note The program execution will be blocked inside this function for as long as the chip is in Deep Power Down mode.
 *  After the chip exits Deep Power Down mode, the execution is NOT resumed and this function will NOT return. Instead,
 *  a reset will occur and the execution will be restarted from the reset handler.
 * @note All information in SRAM will be lost once this function is executed. If relevant data must be stored in the
 *  PMU retained data section (see #Chip_PMU_SetRetainedData) it must be done beforehand.
 * @note The observer flags that indicate which power modes were entered are cleared before entering this standby mode.
 *  Refer to @b Power @b Modes section for more details on the Deep Power Down mode.
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 * @warning Setting @c enableSwitching to 'true' has impact on power consumption during Deep Power Down, but might have
 *  other consequences. See @ref pmu_power_source_switches_par "Power Source switches" chapter.
 * @warning Calling this function at the wrong moment can "brick your IC". See @ref pmu_dpdwarning_anchor "warning" for
 *  more details.
 */
void Chip_PMU_PowerMode_EnterDeepPowerDown(bool enableSwitching);

/**
 * Retrieves the reason for the last wakeup from Deep Power Down.
 * @warning Changing the power mode before calling this function will reset the register #NSS_PMU_T.PCON which maintains
 *  the wakeup reason. This means the wakeup reason from Deep Power Down is lost
 *  - after a reset: POR event or RESETN pin toggling
 *  - when going to Sleep or Deep sleep low power mode: #Chip_PMU_PowerMode_EnterSleep or
 *      #Chip_PMU_PowerMode_EnterDeepSleep
 * @return An enumeration value with the wakeup reason. This value will be #PMU_DPD_WAKEUPREASON_NONE when the register
 *  #NSS_PMU_T.PCON has been reset.
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
PMU_DPD_WAKEUPREASON_T Chip_PMU_PowerMode_GetDPDWakeupReason(void);

/**
 * Gets the status of the VDDBAT pin power source switch
 * @return True if the switch is closed, false otherwise.
 * @note Please refer to detailed documentation section for more details on how the automatic switching mechanism works.
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
bool Chip_PMU_Switch_GetVDDBat(void);

/**
 * Gets the status of the VNFC power source switch
 * @return True if the switch is closed, false otherwise.
 * @note Please refer to detailed documentation section for more details on how the automatic switching mechanism works.
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
bool Chip_PMU_Switch_GetVNFC(void);

/**
 * Forces the VDDBAT switch to its initial open state
 * @note The VBAT switch connects the VDDBAT pin to the VDD_ALON power domain.
 * @post The IC will enter the "Off" state if no power is present in the VNFC power domain or switch to VNFC power
 *  otherwise. See @ref pmu_offmode_anchor "Off mode" for more details.
 * @warning This function performs two synchronized register accesses. Impact on runtime performance and other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_Switch_OpenVDDBat(void);

bool Chip_PMU_Swtich_GetBOD(void);

/**
 * Enables/Disables the Brown-Out detection
 * @param enabled : If set to true enables the BOD, otherwise it disables it
 * @warning This function performs two synchronized register accesses. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_SetBODEnabled(bool enabled);

/**
 * Gets the Brown-Out detection enabled status
 * @return If true the Brown-Out detection is enabled, otherwise it is disabled
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 * lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
bool Chip_PMU_GetBODEnabled(void);

/**
 * Enables/Disables the wakeup pin functionality
 * @param enabled : If set to true enables the wakeup pin functionality, otherwise it disables it
 * @note For more details on how the wakeup pin is configured, refer to the "Wakeup Pin" section
 * @warning This function performs two synchronized register accesses. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_SetWakeupPinEnabled(bool enabled);

/**
 * Gets the wakeup pin functionality enabled status
 * @return If true the wakeup pin functionality is enabled, otherwise it is disabled
 * @note For more details on how the wakeup pin is configured, refer to the "Wakeup Pin" section
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 * lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
bool Chip_PMU_GetWakeupPinEnabled(void);

/**
 * Sets the RTC block clock source. As there is only one possible source, this function effectively allows you to
 * disable all RTC functionality. Gating the clock by providing #PMU_RTC_CLOCKSOURCE_NONE as
 * value for @c source will disable both the up @b and the down counter in the RTC, i.e. will disable the calendaring
 * functionality (it becomes impossible to keep track of absolute time) @b and the wake-up possibility from Deep Power
 * Down mode after a configured number of seconds.
 * Gating the clock for the RTC will save a modest amount of power (in the order of 20 nA).
 * @param source : see #PMU_DPD_WAKEUPREASON_T
 * @note The default RTC block clock source is the TFRO: #PMU_RTC_CLOCKSOURCE_TFRO.
 * @warning This function performs two synchronized register accesses. Impact on runtime performance and other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_SetRTCClockSource(PMU_RTC_CLOCKSOURCE_T source);

/**
 * Gets the RTC block clock source.
 * @return The clock source of the RTC block
 * @note The default RTC block clock source is the TFRO: #PMU_RTC_CLOCKSOURCE_TFRO.
 * @warning This function performs a synchronized register access. Impact on runtime performance and other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
PMU_RTC_CLOCKSOURCE_T Chip_PMU_GetRTCClockSource(void);

/**
 * Sets the required data words into the retained data section of the PMU
 * @param pData : A pointer to the data words to be set
 * @param offset : The offset, in number of words, to where to set the first word
 * @param size : The size, in words, of the data to set
 * @note The content of the retained data section of the PMU will only be lost after a POR or an external reset
 *  (RESETN pin asserted).
 * @note The retained data section length is 5 words, thus, the function will assert that the offset + size does not
 *  exceed 5 words.
 * @warning This function performs "size" synchronized register accesses. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_SetRetainedData(uint32_t *pData, int offset, int size);

/**
 * Gets the required data words stored in the retained data section of the PMU
 * @param pData : A pointer to where to place the retrieved data words
 * @param offset : The offset, in number of words, to where to get the first word from
 * @param size : The size, in words, of the data to get
 * @note The retained data section length is 5 words, thus, the function will assert that the offset + size does not
 *  exceed 5 words.
 * @warning This function performs "size" synchronized register accesses. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_GetRetainedData(uint32_t *pData, int offset, int size);

/**
 * Returns the status information from the PMU block
 * @return Status conditions currently asserted in the PMU block
 * @note A bit set to 1 means that the corresponding status condition is present
 * @note This function will provide the current status bit of the PMU block (once the conditions is not present anymore,
 *  the status bit will be cleared).
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
PMU_STATUS_T Chip_PMU_GetStatus(void);

/**
 * Enables/Disables the PMU interrupts
 * @param mask : interrupt enabled mask to set
 * @warning This function performs two synchronized register accesses. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_Int_SetEnabledMask(PMU_INT_T mask);

/**
 * Retrieves the PMU interrupt enabled mask
 * @return Interrupt enabled mask
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
PMU_INT_T Chip_PMU_Int_GetEnabledMask(void);

/**
 * Retrieves a bitVector with the RAW PMU interrupt flags
 * @return BitVector with the PMU RAW interrupt flags
 * @note A bit set to 1 means that the corresponding interrupt flag is set.
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
PMU_INT_T Chip_PMU_Int_GetRawStatus(void);

/**
 * Clears the required PMU interrupt flags
 * @param flags : Bitvector indicating which interrupt flags to clear
 * @warning This function performs a synchronized register access. Impact on runtime performance in other same and
 *  lower-priority contexts should be carefully considered. See @ref pmu_syncwait_warning_anchor "warning" section.
 */
void Chip_PMU_Int_ClearRawStatus(PMU_INT_T flags);

/**
 * @}
 */

#endif /* __PMU_NSS_H_ */
