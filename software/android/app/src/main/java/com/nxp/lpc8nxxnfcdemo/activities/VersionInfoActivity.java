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


import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.os.Bundle;
import android.widget.ImageView;
import android.widget.LinearLayout;

import com.nxp.lpc8nxxnfcdemo.reader.Nfc_lpc8nxx_Demo;
import com.nxp.lpc8nxxnfcdemo.R;

public class VersionInfoActivity extends Activity {
	private PendingIntent pendingIntent;
	private NfcAdapter mAdapter;
	private LinearLayout versionInformation;
	private ImageView imageVersion;
	private LinearLayout layoutRead;
    private boolean VersionInfoExpanded = false;
	private Nfc_lpc8nxx_Demo demo;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_versioninfo);
		
		layoutRead = (LinearLayout) findViewById(R.id.lconf_ver);

		// Capture intent to check whether the operation should be automatically launch or not
		Tag tag = getIntent().getParcelableExtra(NfcAdapter.EXTRA_TAG);

		// Add Foreground dispatcher
		mAdapter = NfcAdapter.getDefaultAdapter(this);
		pendingIntent = PendingIntent.getActivity(this, 0, new Intent(this,
				getClass()).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);
		return; // end onCreate
	}

	@Override
	public void onPause() {
		super.onPause();
		if (mAdapter != null) {
            mAdapter.disableForegroundDispatch(this);
        }
	}

	@Override
	public void onBackPressed() {
		Intent output = new Intent();
		setResult(RESULT_OK, output);
		finish();
	}

	@Override
	public void onResume() {
		super.onResume();
		if (mAdapter != null) {
            mAdapter.enableForegroundDispatch(this, pendingIntent, null, null);
        }
	}
}