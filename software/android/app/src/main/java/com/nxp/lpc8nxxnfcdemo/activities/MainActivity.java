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
package com.nxp.lpc8nxxnfcdemo.activities;

import java.util.Locale;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.net.Uri;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.os.Bundle;
import android.os.Vibrator;
import android.support.v4.app.FragmentActivity;
import android.support.v4.view.ViewPager;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TabHost;
import android.widget.TabHost.OnTabChangeListener;

import com.nxp.lpc8nxxnfcdemo.adapters.TabsAdapter;
/*import com.nxp.lpc8nxxnfcdemo.fragments.ConfigFragment;*/
import com.nxp.lpc8nxxnfcdemo.fragments.NdefFragment;
import com.nxp.lpc8nxxnfcdemo.reader.Nfc_lpc8nxx_Demo;
import com.nxp.lpc8nxxnfcdemo.R;

public class MainActivity extends FragmentActivity {
	public final static String EXTRA_MESSAGE = "com.nxp.lpc8nxxnfcclock.MESSAGE";
	public static Nfc_lpc8nxx_Demo demo;
	private TabHost mTabHost;
	private ViewPager mViewPager;
	private TabsAdapter mTabsAdapter;
	private PendingIntent mPendingIntent;
	private NfcAdapter mAdapter;
	public static String PACKAGE_NAME;

	// Android app Version
	private static String appVersion = "";

	// Board firmware Version
	private static String boardFirmwareVersion = "";

	private static Intent mIntent;

	public static Intent getmIntent() {
		return mIntent;
	}

	public static void setBoardFirmwareVersion(String boardFirmwareVersion) {
		MainActivity.boardFirmwareVersion = boardFirmwareVersion;
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		// Application package name to be used by the AAR record
		PACKAGE_NAME = getApplicationContext().getPackageName();
		String languageToLoad = "en";
		Locale locale = new Locale(languageToLoad);
		Locale.setDefault(locale);
		Configuration config = new Configuration();
		config.locale = locale;
		getBaseContext().getResources().updateConfiguration(config,
				getBaseContext().getResources().getDisplayMetrics());
		setContentView(R.layout.activity_main);
		mTabHost = (TabHost) findViewById(android.R.id.tabhost);
		mTabHost.setup();
		mViewPager = (ViewPager) findViewById(R.id.pager);
		mTabsAdapter = new TabsAdapter(this, mTabHost, mViewPager);
		mTabsAdapter.addTab(
				mTabHost.newTabSpec("Clock and Temperature").setIndicator(
						getString(R.string.PageTimeTemperature)), NdefFragment.class, null);
		// set current Tag to the Speedtest, so it loads the values
		if (savedInstanceState != null) {
			mTabHost.setCurrentTabByTag(savedInstanceState.getString("tab"));
		}
		// Get App version
		appVersion = "";
		try {
			PackageInfo pInfo = getPackageManager().getPackageInfo(
					getPackageName(), 0);
			appVersion = pInfo.versionName;
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}

		// Board firmware version
		boardFirmwareVersion = "Unknown";

		// Notifier to be used for the demo changing
		mTabHost.setOnTabChangedListener(new OnTabChangeListener() {
			@Override
			public void onTabChanged(String tabId) {
				if (demo.isReady()) {
					demo.finishAllTasks();
					if(demo.isConnected())
					{
						launchDemo(tabId);
					}
				}
				mTabsAdapter.onTabChanged(tabId);
			}
		});

		// Initialize the demo in order to handle tab change events
		demo = new Nfc_lpc8nxx_Demo(null, this);
		mAdapter = NfcAdapter.getDefaultAdapter(this);
		setNfcForeground();
		checkNFC();
	}

	@SuppressLint("InlinedApi")
	private void checkNFC() {
		if (mAdapter != null) {
			if (!mAdapter.isEnabled()) {
				new AlertDialog.Builder(this)
						.setTitle("NFC not enabled")
						.setMessage("Go to Settings?")
						.setPositiveButton("Yes",
								new DialogInterface.OnClickListener() {
									@Override
									public void onClick(DialogInterface dialog,
											int which) {
										 if (android.os.Build.VERSION.SDK_INT >= 16) {
											 startActivity(new Intent(android.provider.Settings.ACTION_NFC_SETTINGS));
										 } else {
											 startActivity(new Intent(android.provider.Settings.ACTION_WIRELESS_SETTINGS));
										 }
									}
								})
						.setNegativeButton("No",
								new DialogInterface.OnClickListener() {
									@Override
									public void onClick(DialogInterface dialog,
											int which) {
										System.exit(0);
									}
								}).show();
			}
		} else {
			new AlertDialog.Builder(this)
					.setTitle("No NFC available. App is going to be closed.")
					.setNeutralButton("Ok",
							new DialogInterface.OnClickListener() {
								@Override
								public void onClick(DialogInterface dialog,
										int which) {
									System.exit(0);
								}
							}).show();
		}
	}

	@Override
	public void onPause() {
		super.onPause();
		if (mAdapter != null) {
			mAdapter.disableForegroundDispatch(this);
		}
				
		if (demo.isReady()) {
			demo.finishAllTasks();
		}
	}

	@Override
	public void onResume() {
		super.onResume();
		
		if (mAdapter != null) {
			mAdapter.enableForegroundDispatch(this, mPendingIntent, null, null);
		}
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		mIntent = null;
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
	    // Check which request we're responding to
		// Make sure the request was successful
		if (resultCode == RESULT_OK
			&& demo != null
			&& demo.isReady()) {
			String currTab = mTabHost.getCurrentTabTag();
			launchDemo(currTab);
	    }
	}

	@Override
	protected void onNewIntent(Intent nfc_intent) {
		super.onNewIntent(nfc_intent);
		// Set the pattern for vibration
		long pattern[] = { 0, 100 };

		// Vibrate on new Intent
		Vibrator vibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
		vibrator.vibrate(pattern, -1);
		doProcess(nfc_intent);
	}

	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.menu, menu);

		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle presses on the action bar items
		switch (item.getItemId()) {
		case R.id.action_about:
			showAboutDialog();
			return true;
		case R.id.action_help:
			showHelpDialog();
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	public void doProcess(Intent nfc_intent) {
		mIntent = nfc_intent;
		Tag tag = nfc_intent.getParcelableExtra(NfcAdapter.EXTRA_TAG);
		demo = new Nfc_lpc8nxx_Demo(tag, this);
		if (demo.isReady()) {
			String currTab = mTabHost.getCurrentTabTag();
			launchDemo(currTab);
		}
	}

	private void launchDemo(String currTab) {

		// ===========================================================================
		// CLock and temperature Demo
		// ===========================================================================
		if (currTab.equalsIgnoreCase(getString(R.string.PageTimeTemperature))) {
// MGN				NdefFragment.setAnswer("Tag detected");
				try {
					demo.NDEF();
				} catch (Exception e) {
					// NdefFragment.setAnswer(getString(R.string.Tag_lost));
				}
		}
	}

	/**
	 * NDEF Demo execution is launched from its fragmend.
	 */
	public static void launchNdefDemo() {
		if (demo.isReady()) {
			if (demo.isConnected()) {
// MGN				NdefFragment.setAnswer("Tag detected");
				try {
					demo.NDEF();
				} catch (Exception e) {
// MGN					NdefFragment.setAnswer("Tag lost, try again");
					e.printStackTrace();
				}
			} else {
				if(NdefFragment.isWriteChosen()) {
// MGN					NdefFragment.setAnswer("Tap tag to write NDEF content");
				} else {
// MGN					NdefFragment.setAnswer("Tap tag to read NDEF content");
				}
			}
		}
	}


	public void showHelpDialog() {
		Intent intent = null;
		intent = new Intent(this, HelpActivity.class);
		startActivity(intent);
	}


	public void showAboutDialog() {
		Intent intent = null;
		intent = new Intent(this, VersionInfoActivity.class);
		if(MainActivity.mIntent != null)
			intent.putExtras(MainActivity.mIntent);

		startActivity(intent);
	}


	public void setNfcForeground() {
		// Create a generic PendingIntent that will be delivered to this
		// activity. The NFC stack will fill
		// in the intent with the details of the discovered tag before
		// delivering it to this activity.
		mPendingIntent = PendingIntent.getActivity(this, 0, new Intent(
				getApplicationContext(), getClass())
				.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);
	}

	
	public static Intent getNfcIntent() {
		return mIntent;
	}
	
	public static void setNfcIntent(Intent intent) {
		mIntent = intent;
	}
}
