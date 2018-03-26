/*
 ****************************************************************************
 * Copyright(c) 2017 NXP Semiconductors                                     *
 * All rights are reserved.                                                 *
 *                                                                          *
 * Software that is described herein is for illustrative purposes only.     *
 * This software is supplied "AS IS" without any warranties of any kind,    *
 * and NXP Semiconductors disclaims any and all warranties, express or      *
 * implied, including all implied warranties of merchantability,            *
 * fitness for a particular purpose and non-infringement of intellectual    *
 * property rights.  NXP Semiconductors assumes no responsibility           *
 * or liability for the use of the software, conveys no license or          *
 * rights under any patent, copyright, mask work right, or any other        *
 * intellectual property rights in or to any products. NXP Semiconductors   *
 * reserves the right to make changes in the software without notification. *
 * NXP Semiconductors also makes no representation or warranty that such    *
 * application will be suitable for the specified use without further       *
 * testing or modification.                                                 *
 *                                                                          *
 * Permission to use, copy, modify, and distribute this software and its    *
 * documentation is hereby granted, under NXP Semiconductors' relevant      *
 * copyrights in the software, without fee, provided that it is used in     *
 * conjunction with NXP Semiconductor products(UCODE I2C, NTAG I2C, LPC8N04)*
 * This  copyright, permission, and disclaimer notice must appear in all    *
 * copies of this code.                                                     *
 ****************************************************************************
 */
package com.nxp.lpc8nxxnfcdemo.fragments;

import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.nxp.lpc8nxxnfcdemo.activities.MainActivity;
import com.nxp.lpc8nxxnfcdemo.R;

//MGN
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;
import java.math.BigDecimal;


import android.widget.TimePicker;
import android.widget.DatePicker;
import android.widget.ToggleButton;


import android.os.Handler;

public class NdefFragment extends Fragment implements OnClickListener,
		OnCheckedChangeListener {
// MGN	private TextView ndefPerformance;
// MGN	private static CheckBox ndefReadLoop;

	private static boolean     writeChosen = false;
	private static int          progressValue = 0;

	private static Timer        Timer_Refresh;
	private static TimerTask    TimerTask_Refresh;
	private static Handler      Handler_Refresh;

	/* Time and Date edit */
	private static EditText     Edit_DateTime;
	public  static String       SendStrTemp = " ";

	/* Alarm Settings */
	private static ToggleButton Button_AlarmEn;		// Alarm enable or not button
	private static TimePicker   Picker_Alarm;			// Alarm picker controller
	private static int          s_alarmhour;			// Alarm hour setting value
	private static int          s_alarmmin;			// Alarm min  setting value
	private static int          s_alarmenable; 		// Alarm enable status
	private static char         c_alarmenable; 		// Alarm enable status

	/* LEDs Settings */
	private static EditText     Edit_TextTempStep;
	private static EditText     Edit_TextTempBase;

	private static SeekBar      SeekBar_TempStep;
	private static SeekBar      SeekBar_TempBase;

	/* Temperature Settings */
	private static ToggleButton Button_TempUnit;
	private static TextView     thermo_c;             // Display
	private static TextView     thermo_f;             // Display
	private static EditText     Edit_TempSlot;        // Temperature Record Time
	private static SeekBar      SeekBar_TempTimeSlot; // Temperature Record Time SeekBar
	private static ToggleButton Button_ReadEn;        // Read or Config Mode switch button

	private static int          s_temperunit;         // Temperature Unit 0-oC or 1-F
	private static char         c_temperunit;         // Temperature Unit C-oC or F-F
	private static char         c_tempsampleperiod;	// Set temperature storage period
	private static char         c_LEDSstep;	        // Set LEDs step
	private static char         c_LEDSbase;	        // Set LEDs base
	private static int          s_temperdata = 0;	    // Temperature value from Tag
	private static double       f_temperdata = 0;	    // Temperature value convert from s_temperadata

	private static double 	  f_temperatureC;	    // Current Temperature value in Celsius
	private static double 	  f_temperatureF;	    // Current Temperature value in Fahrenheit

	private static int          temperature0 = 0;	    // Temperature History 0
	private static int          temperature1 = 0;	    // Temperature History 1
	private static int          temperature2 = 0;	    // Temperature History 2
	private static int          temperature3 = 0;	    // Temperature History 3
	private static int          temperature4 = 0;	    // Temperature History 4
	private static int          temperature5 = 0;	    // Temperature History 5

	/* send string from phone to tag */
	private static EditText     Edit_CmdString;
	private static String       SendString;

	/* Other parameters */
	private static double      f_voltage;                    /* Backup for battery voltage */
	private static int         s_RefreshTimeCnt = 0;

	View viewTemp;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setRetainInstance(true);

		Timer_Refresh     = new Timer();
		Handler_Refresh   = new Handler();
		TimerTask_Refresh = new TimerTask()
		{
			@Override
			public void run() {
				Handler_Refresh.post(new Runnable()
				{
					@Override
					public void run()
					{
						refreshPage(); // Refresh Edit content, like date and time values
					}
				});
			}
		};
		Timer_Refresh.schedule(TimerTask_Refresh, 500, 1 * 500);
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View layout = inflater
				.inflate(R.layout.page_time_temperature, container, false);

// MGN		ndefReadLoop.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
// MGN			@Override
// MGN			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
// MGN				// Read content
// MGN				if (isChecked == true && MainActivity.demo.isReady()) {
// MGN					MainActivity.demo.finishAllTasks();
// MGN					MainActivity.launchNdefDemo();
// MGN				}
// MGN			}
// MGN		});
// MGN		readNdefButton.setOnClickListener(this);
// MGN		writeNdefButton.setOnClickListener(this);
// MGN		ndefWriteOptions.setOnCheckedChangeListener(this);

		// Set the variable to false to avoid sending the last selected value
		writeChosen = true;

		f_voltage      = 0;

		s_alarmenable =  0;
		c_alarmenable =	'D';

		f_temperatureC = 0;
		f_temperatureF = 0;
		s_temperunit  =  1;
		c_temperunit  = 'C';
		c_tempsampleperiod = '2';

		temperature0 = 0;
		temperature1 = 0;
		temperature2 = 0;
		temperature3 = 0;
		temperature4 = 0;
		temperature5 = 0;

		c_LEDSstep = '0';
		c_LEDSbase = '0';
		c_tempsampleperiod = '2';

		s_RefreshTimeCnt= 0;

		initVariables(layout);
		return layout;
	}

	private void initVariables(View layout)
	{

// ------------------------------------------------------------------- //
		SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		Date curDate =  new Date(System.currentTimeMillis());
		Edit_DateTime = (EditText)layout.findViewById(R.id.editText_DateTime);
		SendStrTemp = (formatter.format(curDate)).toString();
		Edit_DateTime.setGravity(Gravity.CENTER);
		Edit_DateTime.setFocusableInTouchMode(false);
		Edit_DateTime.setText(SendStrTemp);
// ------------------------------------------------------------------- //
		Picker_Alarm = (TimePicker)layout.findViewById(R.id.timePicker_Alarm);
		// Hide AM/PM display
		Picker_Alarm.setIs24HourView(true);
		Picker_Alarm.setOnTimeChangedListener(new TimePicker.OnTimeChangedListener() {
			@Override
			public void onTimeChanged(TimePicker view, int hourOfDay, int minute) {
				s_alarmhour = hourOfDay;
				s_alarmmin  = minute+1;
			}
		});

		s_alarmhour = Picker_Alarm.getCurrentHour();
		s_alarmmin  = Picker_Alarm.getCurrentMinute();

		Button_AlarmEn  = (ToggleButton)layout.findViewById(R.id.toggleButtonAlarm);
		Button_AlarmEn.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
				// alarm enable or disable
				if(b == true) {
					c_alarmenable = 'E';
					s_alarmenable = 1;
				}
				else {
					c_alarmenable = 'D';
					s_alarmenable = 0;
				}
			}
		});
// LED Steps and Base ------------------------------------------------------------- //
		Edit_TextTempStep = (EditText)layout.findViewById(R.id.editTextTempStep);
		Edit_TextTempStep.setFocusableInTouchMode(false);

		Edit_TextTempBase = (EditText)layout.findViewById(R.id.editTextTempBase);
		Edit_TextTempBase.setFocusableInTouchMode(false);

		SeekBar_TempStep = (SeekBar)layout.findViewById(R.id.seekBarTempStep);
		SeekBar_TempStep.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			@Override
			public void onStopTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub
			}

			@Override
			public void onStartTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub
			}

			@Override
			public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
				// TODO Auto-generated method stub
				if(s_temperunit == 1) {
					if(arg1 == 0) {
						c_LEDSstep = '0';		// 0.5oC
						Edit_TextTempStep.setText("LED Step: 0.5℃");
					}
					if(arg1 == 1) {
						c_LEDSstep = '1';		// 1.0oC
						Edit_TextTempStep.setText("LED Step: 1.0℃");
					}
					if(arg1 == 2) {
						c_LEDSstep = '2';		// 1.5oC
						Edit_TextTempStep.setText("LED Step: 1.5℃");
					}
					if(arg1 == 3) {
						c_LEDSstep = '3';		// 2.0oC
						Edit_TextTempStep.setText("LED Step: 2.0℃");
					}
				}
				else {
					if(arg1 == 0) {
						c_LEDSstep = '0';		// 1℉
						Edit_TextTempStep.setText("LED Step: 1℉");
					}
					if(arg1 == 1) {
						c_LEDSstep = '1';		// 2℉
						Edit_TextTempStep.setText("LED Step: 2℉");
					}
					if(arg1 == 2) {
						c_LEDSstep = '2';		// 3℉
						Edit_TextTempStep.setText("LED Step: 3℉");
					}
					if(arg1 == 3) {
						c_LEDSstep = '3';		// 4℉
						Edit_TextTempStep.setText("LED Step: 4℉");
					}
				}
			}
		});

		SeekBar_TempBase = (SeekBar)layout.findViewById(R.id.seekBarTempBase);
		SeekBar_TempBase.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			@Override
			public void onStopTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub

			}

			@Override
			public void onStartTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub

			}

			@Override
			public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
				// TODO Auto-generated method stub
				if(s_temperunit == 1) {
					if(arg1 == 0) {
						c_LEDSbase = '0';		// 20oC
						Edit_TextTempBase.setText("LED Base: 20℃");
					}
					if(arg1 == 1) {
						c_LEDSbase = '1';		// 21oC
						Edit_TextTempBase.setText("LED Base: 21℃");
					}
					if(arg1 == 2) {
						c_LEDSbase = '2';		// 22oC
						Edit_TextTempBase.setText("LED Base: 22℃");
					}
					if(arg1 == 3) {
						c_LEDSbase = '3';		// 23oC
						Edit_TextTempBase.setText("LED Base: 23℃");
					}
					if(arg1 == 4) {
						c_LEDSbase = '4';		// 24oC
						Edit_TextTempBase.setText("LED Base: 24℃");
					}
					if(arg1 == 5) {
						c_LEDSbase = '5';		// 25oC
						Edit_TextTempBase.setText("LED Base: 25℃");
					}
					if(arg1 == 6) {
						c_LEDSbase = '6';		// 26oC
						Edit_TextTempBase.setText("LED Base: 26℃");
					}
					if(arg1 == 7) {
						c_LEDSbase = '7';		// 27oC
						Edit_TextTempBase.setText("LED Base: 27℃");
					}
					if(arg1 == 8) {
						c_LEDSbase = '8';		// 28oC
						Edit_TextTempBase.setText("LED Base: 28℃");
					}
					if(arg1 == 9) {
						c_LEDSbase = '9';		// 29oC
						Edit_TextTempBase.setText("LED Base: 29℃");
					}
				}
				else {
					if(arg1 == 0) {
						c_LEDSbase = '0';		// 68℉ -- 20℃
						Edit_TextTempBase.setText("LED Base: 68℉");
					}
					if(arg1 == 1) {
						c_LEDSbase = '1';		// 21oC
						Edit_TextTempBase.setText("LED Base: 70℉");
					}
					if(arg1 == 2) {
						c_LEDSbase = '2';		// 22oC
						Edit_TextTempBase.setText("LED Base: 72℉");
					}
					if(arg1 == 3) {
						c_LEDSbase = '3';		// 23oC
						Edit_TextTempBase.setText("LED Base: 74℉");
					}
					if(arg1 == 4) {
						c_LEDSbase = '4';		// 24oC
						Edit_TextTempBase.setText("LED Base: 76℉");
					}
					if(arg1 == 5) {
						c_LEDSbase = '5';		// 25oC
						Edit_TextTempBase.setText("LED Base: 78℉");
					}
					if(arg1 == 6) {
						c_LEDSbase = '6';		// 26oC
						Edit_TextTempBase.setText("LED Base: 80℉");
					}
					if(arg1 == 7) {
						c_LEDSbase = '7';		// 27oC
						Edit_TextTempBase.setText("LED Base: 82℉");
					}
					if(arg1 == 8) {
						c_LEDSbase = '8';		// 28oC
						Edit_TextTempBase.setText("LED Base: 84℉");
					}
					if(arg1 == 9) {
						c_LEDSbase = '9';		// 29oC
						Edit_TextTempBase.setText("LED Base: 86℉");
					}
				}
			}
		});

// Temperature ------------------------------------------------------------------- //
		viewTemp = (View)layout.findViewById(R.id.layoutView);
		thermo_c = (TextView)layout.findViewById(R.id.thermo_c);
		thermo_f = (TextView)layout.findViewById(R.id.thermo_f);

		f_temperdata = s_temperdata/10;
		thermo_c.setText(10.0 + "");
		thermo_f.setText(10.0*1.8+32 + "");

		Edit_TempSlot = (EditText)layout.findViewById(R.id.editText_TempRecord);
		Edit_TempSlot.setFocusableInTouchMode(false);
		Edit_TempSlot.setText("1 minitues".toString());

		SeekBar_TempTimeSlot = (SeekBar)layout.findViewById(R.id.seekBar_TempSet);
		SeekBar_TempTimeSlot.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			@Override
			public void onStopTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub
			}

			@Override
			public void onStartTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub
			}

			@Override
			public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
				// TODO Auto-generated method stub
				if(arg1 == 0) {
					c_tempsampleperiod = '1';
					Edit_TempSlot.setText("5 seconds");
				}
				if(arg1 == 1) {
					c_tempsampleperiod = '2';
					Edit_TempSlot.setText("1 minitues");
				}
				if(arg1 == 2) {
					c_tempsampleperiod = '3';
					Edit_TempSlot.setText("5 minitues");
				}
			}
		});

		Button_TempUnit = (ToggleButton)layout.findViewById(R.id.toggleButtonTempUnit);
		Button_TempUnit.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
				// temperature unit change
				if(b == true) {
					s_temperunit = 1;
					c_temperunit = 'C';

					if( c_LEDSstep == '0') {
						Edit_TextTempStep.setText("LED Step: 0.5℃");
					}
					if( c_LEDSstep == '1') {
						Edit_TextTempStep.setText("LED Step: 1.0℃");
					}
					if( c_LEDSstep == '2') {
						Edit_TextTempStep.setText("LED Step: 1.5℃");
					}
					if( c_LEDSstep == '3') {
						Edit_TextTempStep.setText("LED Step: 2.0℃");
					}

					if( c_LEDSbase == '0') {
						Edit_TextTempBase.setText("LED Base: 20℃");
					}
					if( c_LEDSbase == '1') {
						Edit_TextTempBase.setText("LED Base: 21℃");
					}
					if( c_LEDSbase == '2') {
						Edit_TextTempBase.setText("LED Base: 22℃");
					}
					if( c_LEDSbase == '3') {
						Edit_TextTempBase.setText("LED Base: 23℃");
					}
					if( c_LEDSbase == '4') {
						Edit_TextTempBase.setText("LED Base: 24℃");
					}
					if( c_LEDSbase == '5') {
						Edit_TextTempBase.setText("LED Base: 25℃");
					}
					if( c_LEDSbase == '6') {
						Edit_TextTempBase.setText("LED Base: 26℃");
					}
					if( c_LEDSbase == '7') {
						Edit_TextTempBase.setText("LED Base: 27℃");
					}
					if( c_LEDSbase == '8') {
						Edit_TextTempBase.setText("LED Base: 28℃");
					}
					if( c_LEDSbase == '9') {
						Edit_TextTempBase.setText("LED Base: 29℃");
					}
				}
				else {
					s_temperunit = 0;
					c_temperunit = 'F';

					if( c_LEDSstep == '0') {
						Edit_TextTempStep.setText("LED Step: 1℉");
					}
					if( c_LEDSstep == '1') {
						Edit_TextTempStep.setText("LED Step: 2℉");
					}
					if( c_LEDSstep == '2') {
						Edit_TextTempStep.setText("LED Step: 3℉");
					}
					if( c_LEDSstep == '3') {
						Edit_TextTempStep.setText("LED Step: 4℉");
					}

					if( c_LEDSbase == '0') {
						Edit_TextTempBase.setText("LED Base: 68℉");
					}
					if( c_LEDSbase == '1') {
						Edit_TextTempBase.setText("LED Base: 70℉");
					}
					if( c_LEDSbase == '2') {
						Edit_TextTempBase.setText("LED Base: 72℉");
					}
					if( c_LEDSbase == '3') {
						Edit_TextTempBase.setText("LED Base: 74℉");
					}
					if( c_LEDSbase == '4') {
						Edit_TextTempBase.setText("LED Base: 76℉");
					}
					if( c_LEDSbase == '5') {
						Edit_TextTempBase.setText("LED Base: 78℉");
					}
					if( c_LEDSbase == '6') {
						Edit_TextTempBase.setText("LED Base: 80℉");
					}
					if( c_LEDSbase == '7') {
						Edit_TextTempBase.setText("LED Base: 82℉");
					}
					if( c_LEDSbase == '8') {
						Edit_TextTempBase.setText("LED Base: 84℉");
					}
					if( c_LEDSbase == '9') {
						Edit_TextTempBase.setText("LED Base: 86℉");
					}
				}
			}
		});
// Read or Config Mode Switch ------------------------------------------------------- //
		Button_ReadEn  = (ToggleButton)layout.findViewById(R.id.toggleButton_Read);
		Button_ReadEn.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
				// read or write
				if( (b == true) && MainActivity.demo.isReady() ) {
					writeChosen = false;
				}
				else {
					writeChosen = true;
				}
			}
		});
// Command String ------------------------------------------------------------------- //
		Edit_CmdString = (EditText)layout.findViewById(R.id.editText_CmdString);
		Edit_CmdString.setFocusableInTouchMode(false);
		// Just as template strings, will update this every 500mS in Refresh function
		SendStrTemp = ("NEW"+formatter.format(curDate)+"END").toString();
		//SendString = SendStrTemp;
		Edit_CmdString.setText(SendStrTemp);
// ------------------------------------------------------------------- //
	}

	public void onClick(View v) {
		switch (v.getId()) {

		default:
			break;
		}
	} // END onClick (View v)


	@Override
	public void onCheckedChanged(RadioGroup group, int checkedId) {

	}
	
	public static void resetNdefDemo() {
		if (writeChosen == true) {
// MGN            setAnswer("Tap tag to write NDEF content");
        } else {
// MGN            setAnswer("Tap tag to read NDEF content");
        }
	}
// MGN	public static boolean isNdefReadLoopSelected() {
// MGN		return ndefReadLoop.isChecked();
// MGN	}

// MGN	public static void setAnswer(String answer) {
// MGN		ndefCallback.setText(answer);
// MGN	}

	public static void refreshPage() {
		/* Refresh Page every 500mS */

		/* Refresh send command string */
		SimpleDateFormat DataFormatRefresh = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		Date curDateRefresh = new Date(System.currentTimeMillis());
		Edit_DateTime.setText((DataFormatRefresh.format(curDateRefresh)).toString());

		int alarmhour = Picker_Alarm.getCurrentHour();
		int alarmmin  = Picker_Alarm.getCurrentMinute();

		SimpleDateFormat formatterAlarm = new SimpleDateFormat("HH:mm");
		Date alarmData =  new Date(0,0,0,alarmhour, alarmmin);

		/* String Str_EditDateTimeStr */
		SendStrTemp = ("NEW"+(DataFormatRefresh.format(curDateRefresh)).toString()+c_alarmenable+" "+(formatterAlarm.format(alarmData)).toString()+" "+c_tempsampleperiod+" "+c_temperunit+" S"+ c_LEDSstep+" B"+ c_LEDSbase ).toString();
		Edit_CmdString.setText(SendStrTemp);

		disp5Tmperature(temperature1, temperature2, temperature3, temperature4, temperature5);

		BigDecimal b = new BigDecimal(((float)temperature0)/10);
		float f1 = b.setScale(1, BigDecimal.ROUND_HALF_UP).floatValue();
		thermo_c.setText(f1 + "");
		thermo_f.setText((float)((float)temperature0*18+3200)/100 + "");

		s_RefreshTimeCnt++;
	}

	public static void disp5Tmperature(int data1,int data2,int data3,int data4,int data5) {
		if(c_tempsampleperiod == '1') {
			String disp_str1;
			disp_str1 = ("5 seconds\r\n"
					+"01: " + (String.valueOf(data1/10))+"."+(String.valueOf(data1%10))+"℃ / "+ (String.valueOf( (data1*18+3200)/10/10))+"."+(String.valueOf( (data1*18+3200)/10%10)) +"F\r\n"
					+"02: " + (String.valueOf(data2/10))+"."+(String.valueOf(data2%10))+"℃ / "+ (String.valueOf( (data2*18+3200)/10/10))+"."+(String.valueOf( (data2*18+3200)/10%10)) +"F\r\n"
					+"03: " + (String.valueOf(data3/10))+"."+(String.valueOf(data3%10))+"℃ / "+ (String.valueOf( (data3*18+3200)/10/10))+"."+(String.valueOf( (data3*18+3200)/10%10)) +"F\r\n"
					+"04: " + (String.valueOf(data4/10))+"."+(String.valueOf(data4%10))+"℃ / "+ (String.valueOf( (data4*18+3200)/10/10))+"."+(String.valueOf( (data4*18+3200)/10%10)) +"F\r\n"
					+"05: " + (String.valueOf(data5/10))+"."+(String.valueOf(data5%10))+"℃ / "+ (String.valueOf( (data5*18+3200)/10/10))+"."+(String.valueOf( (data5*18+3200)/10%10)) +"F\r\n"
			);
			Edit_TempSlot.setText(disp_str1);
		}
		if(c_tempsampleperiod == '2') {
			String disp_str1;
			disp_str1 = ("1 minutes\r\n"
					+"01: " + (String.valueOf(data1/10))+"."+(String.valueOf(data1%10))+"℃ / "+ (String.valueOf( (data1*18+3200)/10/10))+"."+(String.valueOf( (data1*18+3200)/10%10)) +"F\r\n"
					+"02: " + (String.valueOf(data2/10))+"."+(String.valueOf(data2%10))+"℃ / "+ (String.valueOf( (data2*18+3200)/10/10))+"."+(String.valueOf( (data2*18+3200)/10%10)) +"F\r\n"
					+"03: " + (String.valueOf(data3/10))+"."+(String.valueOf(data3%10))+"℃ / "+ (String.valueOf( (data3*18+3200)/10/10))+"."+(String.valueOf( (data3*18+3200)/10%10)) +"F\r\n"
					+"04: " + (String.valueOf(data4/10))+"."+(String.valueOf(data4%10))+"℃ / "+ (String.valueOf( (data4*18+3200)/10/10))+"."+(String.valueOf( (data4*18+3200)/10%10)) +"F\r\n"
					+"05: " + (String.valueOf(data5/10))+"."+(String.valueOf(data5%10))+"℃ / "+ (String.valueOf( (data5*18+3200)/10/10))+"."+(String.valueOf( (data5*18+3200)/10%10)) +"F\r\n"
			);
			Edit_TempSlot.setText(disp_str1);
		}
		if(c_tempsampleperiod == '3') {
			String disp_str1;
			disp_str1 = ("5 minitures\r\n"
					+"01: " + (String.valueOf(data1/10))+"."+(String.valueOf(data1%10))+"℃ / "+ (String.valueOf( (data1*18+3200)/10/10))+"."+(String.valueOf( (data1*18+3200)/10%10)) +"F\r\n"
					+"02: " + (String.valueOf(data2/10))+"."+(String.valueOf(data2%10))+"℃ / "+ (String.valueOf( (data2*18+3200)/10/10))+"."+(String.valueOf( (data2*18+3200)/10%10)) +"F\r\n"
					+"03: " + (String.valueOf(data3/10))+"."+(String.valueOf(data3%10))+"℃ / "+ (String.valueOf( (data3*18+3200)/10/10))+"."+(String.valueOf( (data3*18+3200)/10%10)) +"F\r\n"
					+"04: " + (String.valueOf(data4/10))+"."+(String.valueOf(data4%10))+"℃ / "+ (String.valueOf( (data4*18+3200)/10/10))+"."+(String.valueOf( (data4*18+3200)/10%10)) +"F\r\n"
					+"05: " + (String.valueOf(data5/10))+"."+(String.valueOf(data5%10))+"℃ / "+ (String.valueOf( (data5*18+3200)/10/10))+"."+(String.valueOf( (data5*18+3200)/10%10)) +"F\r\n"
			);
			Edit_TempSlot.setText(disp_str1);
		}
	}

	public static void getTagString(byte[] answer) {
		byte[] temp_data = new byte[128];
		System.arraycopy(answer, 0, temp_data, 0, 128);
		int i, j;
		for(i=0; i<10000; i++) {
			if( (temp_data[i] == 'e') && (temp_data[i+1] == 'n') ) {
				for(j=0; j<1000; j++) {
					if( (temp_data[i+j] == 'T') && (temp_data[i+j+1] == 'E') && (temp_data[i+j+2] == 'M') && (temp_data[i+j+3] == 'P') && (temp_data[i+j+4] == '0') ) {
						temperature0 = temperature1 = temperature2 = temperature3 = temperature4 = temperature5 = 0;
						// Temperature 1
						if(temp_data[i+j+15] != 0x20) {
							temperature1 += (temp_data[i+j+15]-0x30)*10000;
						}
						if(temp_data[i+j+16] != 0x20) {
							temperature1 += (temp_data[i+j+16]-0x30)*1000;
						}
						if(temp_data[i+j+17] != 0x20) {
							temperature1 += (temp_data[i+j+17]-0x30)*100;
						}
						if(temp_data[i+j+18] != 0x20) {
							temperature1 += (temp_data[i+j+18]-0x30)*10;
						}
						if(temp_data[i+j+19] != 0x20) {
							temperature1 += (temp_data[i+j+19]-0x30);
						}
						// Temperature 2
						if(temp_data[i+j+25] != 0x20) {
							temperature2 += (temp_data[i+j+25]-0x30)*10000;
						}
						if(temp_data[i+j+26] != 0x20) {
							temperature2 += (temp_data[i+j+26]-0x30)*1000;
						}
						if(temp_data[i+j+27] != 0x20) {
							temperature2 += (temp_data[i+j+27]-0x30)*100;
						}
						if(temp_data[i+j+28] != 0x20) {
							temperature2 += (temp_data[i+j+28]-0x30)*10;
						}
						if(temp_data[i+j+29] != 0x20) {
							temperature2 += (temp_data[i+j+29]-0x30);
						}
						// Temperature 3
						if(temp_data[i+j+35] != 0x20) {
							temperature3 += (temp_data[i+j+35]-0x30)*10000;
						}
						if(temp_data[i+j+36] != 0x20) {
							temperature3 += (temp_data[i+j+36]-0x30)*1000;
						}
						if(temp_data[i+j+37] != 0x20) {
							temperature3 += (temp_data[i+j+37]-0x30)*100;
						}
						if(temp_data[i+j+38] != 0x20) {
							temperature3 += (temp_data[i+j+38]-0x30)*10;
						}
						if(temp_data[i+j+39] != 0x20) {
							temperature3 += (temp_data[i+j+39]-0x30);
						}
						// Temperature 4
						if(temp_data[i+j+45] != 0x20) {
							temperature4 += (temp_data[i+j+45]-0x30)*10000;
						}
						if(temp_data[i+j+46] != 0x20) {
							temperature4 += (temp_data[i+j+46]-0x30)*1000;
						}
						if(temp_data[i+j+47] != 0x20) {
							temperature4 += (temp_data[i+j+47]-0x30)*100;
						}
						if(temp_data[i+j+48] != 0x20) {
							temperature4 += (temp_data[i+j+48]-0x30)*10;
						}
						if(temp_data[i+j+49] != 0x20) {
							temperature4 += (temp_data[i+j+49]-0x30);
						}
						// Temperature 5
						if(temp_data[i+j+55] != 0x20) {
							temperature5 += (temp_data[i+j+55]-0x30)*10000;
						}
						if(temp_data[i+j+56] != 0x20) {
							temperature5 += (temp_data[i+j+56]-0x30)*1000;
						}
						if(temp_data[i+j+57] != 0x20) {
							temperature5 += (temp_data[i+j+57]-0x30)*100;
						}
						if(temp_data[i+j+58] != 0x20) {
							temperature5 += (temp_data[i+j+58]-0x30)*10;
						}
						if(temp_data[i+j+59] != 0x20) {
							temperature5 += (temp_data[i+j+59]-0x30);
						}
						//disp5Tmperature(temperature1, temperature2, temperature3, temperature4, temperature5);
						if(temp_data[i+j+5] != 0x20) {
							temperature0 += (temp_data[i+j+5]-0x30)*10000;
						}
						if(temp_data[i+j+6] != 0x20) {
							temperature0 += (temp_data[i+j+6]-0x30)*1000;
						}
						if(temp_data[i+j+7] != 0x20) {
							temperature0 += (temp_data[i+j+7]-0x30)*100;
						}
						if(temp_data[i+j+8] != 0x20) {
							temperature0 += (temp_data[i+j+8]-0x30)*10;
						}
						if(temp_data[i+j+9] != 0x20) {
							temperature0 += (temp_data[i+j+9]-0x30);
						}
						//setTemperature(temperature0);
						/* jump out of for(;;) */
						i = 1000000;
						j = 1000000;
					}
				}
			}
		}
	}

	public static boolean isNdefReadLoopSelected() {
		return writeChosen;
	}
	/*  */
	public static boolean isWriteChosen() {
		return writeChosen;
	}

	public static String getSendString() {
		return Edit_CmdString.getText().toString();
	}
}

// end file
