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
package com.nxp.lpc8nxxnfcdemo.reader;

import java.io.IOException;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.Arrays;

import org.ndeftools.EmptyRecord;
import org.ndeftools.Message;
import org.ndeftools.MimeRecord;
import org.ndeftools.Record;
import org.ndeftools.wellknown.TextRecord;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.nfc.FormatException;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.Tag;
import android.nfc.tech.Ndef;
import android.nfc.tech.NfcA;
import android.nfc.tech.NfcB;
import android.nfc.tech.NfcF;
import android.nfc.tech.NfcV;
import android.os.AsyncTask;
import android.widget.Toast;

import com.nxp.lpc8nxxnfcdemo.crypto.CRC32Calculator;
import com.nxp.lpc8nxxnfcdemo.exceptions.CommandNotSupportedException;
import com.nxp.lpc8nxxnfcdemo.fragments.NdefFragment;
import com.nxp.lpc8nxxnfcdemo.listeners.WriteEEPROMListener;
import com.nxp.lpc8nxxnfcdemo.reader.Nfc_Get_Version.Prod;
import com.nxp.lpc8nxxnfcdemo.utils.LEDUtil;
import com.nxp.lpc8nxxnfcdemo.R;

/**
 * Class for the different Demos.
 *
 * @author NXP67729
 *
 */

public class Nfc_lpc8nxx_Demo implements WriteEEPROMListener {

	private Lpc_Enabled_Commands reader;
	private Activity main;
	private Tag tag;

	/**
	 *
	 * Taskreferences.
	 *
	 */
	private WriteEmptyNdefTask emptyNdeftask;
	private WriteDefaultNdefTask defaultNdeftask;
	private NDEFReadTask ndefreadtask;

	/**
	 *
	 * DEFINES.
	 *
	 */
	private static final int LAST_FOUR_BYTES 	= 4;
	private static final int DELAY_TIME 		= 100;
	private static final int TRAILS 			= 300;
	private static final int DATA_SEND_BYTE 	= 12;
	private static final int VERSION_BYTE 		= 63;
	private static final int GET_VERSION_NR 	= 12;
	private static final int GET_FW_NR 			= 28;
	private static final int THREE_BYTES 		= 3;
	private static final int PAGE_SIZE 			= 4096;

	/**
	 * Constructor.
	 *
	 * @param tag
	 *            Tag with which the Demos should be performed
	 * @param main
	 *            MainActivity
	 */
	public Nfc_lpc8nxx_Demo(Tag tag, final Activity main) {
		try {
			if (tag == null) {
				this.main = null;
				this.tag = null;
				return;
			}
			this.main = main;
			this.tag = tag;

			reader = Lpc_Enabled_Commands.get(tag);

			if (reader == null) {
				String message = "The Tag could not be identified or this NFC device does not "
						+ "support the NFC Forum commands needed to access this tag";
				String title = "Communication failed";
				showAlert(message, title);
			} else {
				reader.connect();
			}

			Nfc_Get_Version.Prod prod = reader.getProduct();

			if (!prod.equals(Nfc_Get_Version.Prod.Unknown)) {
				if (prod.equals(Nfc_Get_Version.Prod.NTAG_I2C_1k_Plus)
			     || prod.equals(Nfc_Get_Version.Prod.NTAG_I2C_2k_Plus)) {
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}

	}

	private void showAlert(final String message, final String title) {
		main.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				new AlertDialog.Builder(main)
						.setMessage(message)
						.setTitle(title)
						.setPositiveButton("OK",
								new DialogInterface.OnClickListener() {
									@Override
									public void onClick(DialogInterface dialog,
											int which) {

									}
								}).show();
			}
		});

	}

	/**
	 * Checks if the tag is still connected based on the previously detected reader.
	 *
	 * @return Boolean indicating tag connection
	 *
	 */
	public boolean isConnected() {
		return reader.isConnected();
	}

	/**
	 * Checks if the tag is still connected based on the tag.
	 *
	 * @return Boolean indicating tag presence
	 *
	 */
	public static boolean isTagPresent(Tag tag) {
		final Ndef ndef = Ndef.get(tag);
		if (ndef != null && !ndef.getType().equals("android.ndef.unknown")) {
			try {
				ndef.connect();
				final boolean isConnected = ndef.isConnected();
				ndef.close();
				return isConnected;
			} catch (final IOException e) {
				e.printStackTrace();
				return false;
			}
		} else {
			final NfcA nfca = NfcA.get(tag);
			if (nfca != null) {
				try {
					nfca.connect();
					final boolean isConnected = nfca.isConnected();
					nfca.close();

					return isConnected;
				} catch (final IOException e) {
					e.printStackTrace();
					return false;
				}
			} else {
				final NfcB nfcb = NfcB.get(tag);
				if (nfcb != null) {
					try {
						nfcb.connect();
						final boolean isConnected = nfcb.isConnected();
						nfcb.close();
						return isConnected;
					} catch (final IOException e) {
						e.printStackTrace();
						return false;
					}
				} else {
					final NfcF nfcf = NfcF.get(tag);
					if (nfcf != null) {
						try {
							nfcf.connect();
							final boolean isConnected = nfcf.isConnected();
							nfcf.close();
							return isConnected;
						} catch (final IOException e) {
							e.printStackTrace();
							return false;
						}
					} else {
						final NfcV nfcv = NfcV.get(tag);
						if (nfcv != null) {
							try {
								nfcv.connect();
								final boolean isConnected = nfcv.isConnected();
								nfcv.close();
								return isConnected;
							} catch (final IOException e) {
								e.printStackTrace();
								return false;
							}
						} else {
							return false;
						}
					}
				}
			}
		}
	}

	/**
	 *
	 * Finish all tasks.
	 *
	 */
	public void finishAllTasks() {
		NDEFReadFinish();
	}

	/**
	 * Checks if the demo is ready to be executed.
	 *
	 * @return Boolean indicating demo readiness
	 *
	 */
	public boolean isReady() {
		if (tag != null && reader != null) {
			return true;
		}
		return false;
	}

	/**
	 * Get the product from reader.
	 *
	 * @return product
	 *
	 */
	public Prod getProduct() throws IOException {
		return reader.getProduct();
	}


	/**
	 * Reads the whole tag memory content.
	 * 
	 * @return Boolean indicating success or error
	 */
	public byte[] readTagContent() {
		byte[] bytes = null;
		try {
			// The user memory and the first four pages are displayed
			int memSize = reader.getProduct().getMemsize() + 16;
			// Read all the pages using the fast read method
			bytes = reader.readEEPROM(0, memSize / reader.getBlockSize());
		} catch (IOException e) {
			e.printStackTrace();
		} catch (FormatException e) {
			e.printStackTrace();
		} catch (CommandNotSupportedException e) {
			e.printStackTrace();
			showDemoNotSupportedAlert();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return bytes;
	}
	
	private void showTagNotPlusAlert() {
		String message = main.getString(R.string.tag_not_supported);
		String title = main.getString(R.string.tag_not_supported_title);
		showAlert(message, title);
	}

	private void showDemoNotSupportedAlert() {
		String message = main.getString(R.string.demo_not_supported);
		String title = main.getString(R.string.demo_not_supported_title);
		showAlert(message, title);
	}

	/**
	 * Resets the whole tag memory content (Memory to 00000...)
	 * 
	 * @return Boolean indicating success or error
	 */
	public boolean resetTagContent() {
		boolean success = true;
		try {
			byte[] d = new byte[reader.getProduct().getMemsize()];
			reader.writeEEPROM(d,this);
		} catch (IOException e) {
			success = false;
			e.printStackTrace();
		} catch (FormatException e) {
			success = false;
			e.printStackTrace();
		} catch (CommandNotSupportedException e) {
			showDemoNotSupportedAlert();
			e.printStackTrace();
		}
		return success;
	}



	/**
	 * Stops the NDEFRead Demo
	 */
	public void NDEFReadFinish() {
		if (ndefreadtask != null && !ndefreadtask.isCancelled()) {
			ndefreadtask.exit = true;
			try {
				ndefreadtask.get();
			} catch (Exception e) {
				e.printStackTrace();
			}
			ndefreadtask = null;
			// Clean all the fields
			NdefFragment.resetNdefDemo();
		}
	}

	/* Concatenate two byte arrays*/
	public byte[] concatByteArrays(byte[] s1,byte[]s2)
	{
		byte[] dest = new byte[s1.length + s2.length];

		System.arraycopy(s1, 0, dest, 0, s1.length);
		System.arraycopy(s2, 0, dest, s1.length, s2.length);

		return dest;
	}

	/**
	 * Performs the NDEF Demo
	 */
	public void NDEF() throws IOException, FormatException, CommandNotSupportedException, InterruptedException {
		// Check if the operation is read or write
		if (NdefFragment.isWriteChosen() == true) {

			// NDEF Message to write in the tag
			NdefMessage msg = null;
			msg = createNdefTextMessage(NdefFragment.getSendString());

			nfcReadWriteOperationCheck(); // Check state of nfctag before read write operation

			try {
				long timeToWriteNdef = NDEFWrite(msg);
				Toast.makeText(main, "write tag successfully done", Toast.LENGTH_LONG).show();

				/* Write Idle State once NFC write operation is completed*/
				writeNfcRow(5,false);

				int bytes = msg.toByteArray().length;
				String Message = "";
				
				// Transmission Results
				Message = Message.concat("Speed (" + bytes + " Byte / "
						+ timeToWriteNdef + " ms): "
						+ String.format("%.0f", bytes / (timeToWriteNdef / 1000.0))
						+ " Bytes/s");
// MGN				NdefFragment.setDatarate(Message);
			} catch (Exception e) {
				Toast.makeText(main, "write tag failed", Toast.LENGTH_LONG).show();
// MGN				NdefFragment.setDatarate("Error writing NDEF");
				e.printStackTrace();
			}
		} else {
			ndefreadtask = new NDEFReadTask();
			ndefreadtask.execute();
		}
	}

	public void nfcReadWriteOperationCheck() throws IOException, FormatException, InterruptedException, CommandNotSupportedException {
		int retryCount = 0;
		int MAX_RETRY_COUNT = 1000;
		byte[] tempread = {0};
		for(retryCount = 0; retryCount < MAX_RETRY_COUNT; retryCount++)
		{
			tempread = reader.readEEPROM(4,7); // reads 4 pages always
			if(tempread[0] == (byte)0xFD && tempread[1] == (byte)0x02 && tempread[2] == (byte)0x00 && tempread[3] == (byte)0x00) {
				writeNfcRow(5, true);
				tempread = reader.readEEPROM(4, 7); // reads 4 pages always

				/* If nfc tag on embedded side is idle*/
				if ((tempread[0] == (byte)0xFD && tempread[1] == (byte)0x02 && tempread[2] == (byte)0x00 && tempread[3] == (byte)0x00)
					&& (tempread[4] == (byte) 0xFD && tempread[5] == (byte) 0x02 && tempread[6] == (byte) 0x01 && tempread[7] == (byte) 0x00))
				{
					break;
				} else {	/* if nfc tag is not idle on embedded side*/
					writeNfcRow(5, false);
					Thread.sleep(5); // 5ms

				}
			}else {	/* if nfc tag is not idle on embedded side*/
				Thread.sleep(5);
			}
		}

	}

	public boolean nfcReadScrollOperationCheck() throws IOException, FormatException, InterruptedException, CommandNotSupportedException {

		byte[] tempread ={0};
		boolean stat = false;
		tempread = reader.readEEPROM(4, 7); // reads 4 pages always

		if (tempread[0] == (byte) 0xFD && tempread[1] == (byte) 0x02 && tempread[2] == (byte) 0x00 && tempread[3] == (byte) 0x00) {
			writeNfcRow(5, true);

			tempread[2] = (byte) 0x00;
			tempread = reader.readEEPROM(4, 7); // reads 4 pages always

			/* If nfc tag on embedded side is idle*/
			if ((tempread[0] == (byte) 0xFD && tempread[1] == (byte) 0x02 && tempread[2] == (byte) 0x00 && tempread[3] == (byte) 0x00)
				&& (tempread[4] == (byte) 0xFD && tempread[5] == (byte) 0x02 && tempread[6] == (byte) 0x01 && tempread[7] == (byte) 0x00))
			{
				return true;
			} else {	/* if nfc tag is not idle on embedded side*/
				writeNfcRow(5, false);
				return false;
			}
	} else {	/* if nfc tag is not idle on embedded side*/
			return false;
		}

	}


	/* Write the Second page of NFC memory*/
	public void writeNfcRow(int rowNum, boolean busy) throws IOException, FormatException, CommandNotSupportedException {

		byte[] nfcBusyWriteRow = new byte[4];

		nfcBusyWriteRow[0] = (byte) 0xFD; // proprietary record
		nfcBusyWriteRow[1] = (byte) 0x02; // length
		nfcBusyWriteRow[2] = (byte) 0x01; // Busy
		nfcBusyWriteRow[3] = (byte) 0x00; // reserved

		byte[] nfcIdleWriteRow = new byte[4];
		nfcIdleWriteRow[0] = (byte) 0xFD; // proprietary record
		nfcIdleWriteRow[1] = (byte) 0x02; // length
		nfcIdleWriteRow[2] = (byte) 0x00; // Idle
		nfcIdleWriteRow[3] = (byte) 0x00; // reserved

		if(busy){
			reader.writeEEPROMCustom(nfcBusyWriteRow,rowNum,null, false);
		}else {
			reader.writeEEPROMCustom(nfcIdleWriteRow,rowNum,null,false);
		}

	}
	private class NDEFReadTask extends AsyncTask<Void, String, Void> {

		private Boolean exit = false;

		@Override
		protected Void doInBackground(Void... params) {
			try {
				while (true) {

					// cancel task if requested
					if (exit) {
						cancel(true);
						return null;
					}

					nfcReadWriteOperationCheck(); // Check state of nfctag before read write operation

					long RegTimeOutStart = System.currentTimeMillis();
					// Get the message Type
					//NdefMessage msg = reader.readNDEF();
					byte[] b_tagdata = readTagContent();
					NdefFragment.getTagString(b_tagdata);
					// NDEF Reading time statistics
					long timeToReadNdef = System.currentTimeMillis() - RegTimeOutStart;
					/* Write Idle State once NFC read operation is completed*/
					writeNfcRow(5,false);

					// sleep 500ms, but check if task was cancelled
					for (int i = 0; i < 1; i++) {
						// cancle task if requested
						if (exit || NdefFragment.isNdefReadLoopSelected()) {
							cancel(true);
							return null;
						}
						Thread.sleep(10);
					}

				}
			} catch (CommandNotSupportedException e) {
				e.printStackTrace();
				showAlert(
						"The NDEF Message is to long to read from an "
					  + "NTAG I2C 2K with this Nfc Device",
						"Demo not supported");
			} catch (Exception e) {
				e.printStackTrace();
			}
			return null;
		}

		@Override
		protected void onProgressUpdate(String... progress) {

		}

		@Override
		protected void onPostExecute(Void bla) {
			if (!exit) {
				Toast.makeText(main, main.getString(R.string.Tag_lost),
						Toast.LENGTH_LONG).show();
			}
			NdefFragment.resetNdefDemo();
		}
	}

	/***
	 * Helper method to adjusts the number of bytes to be sent during the last flashing sector
	 * 
	 * @param num to round up
	 * @return number roundep up
	 */
	int roundUp(int num) {
		if(num <= 256) {
			return 256;
		} else if(num > 256 && num <= 512) {
			return 512;
		} else if(num > 512 && num <= 1024) {
			return 1024;
		} else {
			return 4096;
		}
	}


	/**
	 * Appends the CRC32 to the message transmitted during the SRAM SpeedTest.
	 *
	 * @param bytes
	 *            The content to be transmitted to the NTAGI2C board
	 *
	 * @return Byte Array with the CRC32s embedded in it
	 */
	private byte[] appendCRC32(byte[] bytes) {
		byte[] temp = new byte[bytes.length - LAST_FOUR_BYTES];
		System.arraycopy(bytes, 0, temp, 0, temp.length);
		byte[] crc = CRC32Calculator.CRC32(temp);
		System.arraycopy(crc, 0, bytes, bytes.length - crc.length, crc.length);
		return bytes;
	}

	/**
	 * Checks if the CRC32 value has been appended in the message.
	 *
	 * @param bytes
	 *            The whole message received from the board
	 *
	 * @return boolean that indicates the presence of the CRC32
	 */
	private boolean isCRC32Appended(byte[] bytes) {
		for (int i = bytes.length - LAST_FOUR_BYTES; i < bytes.length; i++) {
			if (bytes[i] != 0x00)
				return true;
		}
		return false;
	}

	/**
	 * Checks the received CRC32 value in the message received from the NTAG I2C
	 * board during the SRAM SpeedTest.
	 *
	 * @param bytes
	 *            The whole message received from the board
	 *
	 * @return boolean with the result of the comparison between the CRC32
	 *         received and the CRC32 calculated
	 */
	private boolean isValidCRC32(byte[] bytes) {
		byte[] receivedCRC = { bytes[bytes.length - LAST_FOUR_BYTES],
				bytes[bytes.length - 3],
				bytes[bytes.length - 2],
				bytes[bytes.length - 1] };

		byte[] temp = new byte[bytes.length - LAST_FOUR_BYTES];
		System.arraycopy(bytes, 0, temp, 0, bytes.length - LAST_FOUR_BYTES);
		byte[] calculatedCRC = CRC32Calculator.CRC32(temp);
		return Arrays.equals(receivedCRC, calculatedCRC);
	}


	/**
	 * Write empty ndef task.
	 */
	private class WriteEmptyNdefTask extends AsyncTask<Void, Void, Void> {

		@SuppressWarnings("unused")
		private Boolean exit = false;

		@Override
		protected Void doInBackground(Void... params) {
			try {
				reader.writeEmptyNdef();
			} catch (Exception e) {
				e.printStackTrace();
				cancel(true);
				return null;
			}
			return null;
		}
	}

	/**
	 * Write empty ndef finish.
	 */
	public void WriteEmptyNdefFinish() {
		if (emptyNdeftask != null && !emptyNdeftask.isCancelled()) {
			emptyNdeftask.exit = true;
			try {
				emptyNdeftask.get();
			} catch (Exception e) {
				e.printStackTrace();
			}
			emptyNdeftask = null;
		}
	}

	/**
	 * Write default ndef task.
	 */
	private class WriteDefaultNdefTask extends AsyncTask<Void, Void, Void> {

		@SuppressWarnings("unused")
		private Boolean exit = false;

		@Override
		protected Void doInBackground(Void... params) {
			try {
				reader.writeDefaultNdef();
			} catch (Exception e) {
				e.printStackTrace();
				cancel(true);
				return null;
			}
			return null;
		}
	}

	/**
	 * Write default ndef finish.
	 */
	public void WriteDefaultNdefFinish() {
		if (defaultNdeftask != null && !defaultNdeftask.isCancelled()) {
			defaultNdeftask.exit = true;
			try {
				defaultNdeftask.get();
			} catch (Exception e) {
				e.printStackTrace();
			}
			defaultNdeftask = null;
		}
	}

	/**
	 * Creates a NDEF Text Message.
	 * 
	 * @param text
	 *            Text to write
	 * @return NDEF Message
	 * @throws UnsupportedEncodingException
	 */
	private NdefMessage createNdefTextMessage(String text)
			throws UnsupportedEncodingException {
		if(text.length() == 0) {
			return null;
		}
		String lang = "en";
		byte[] textBytes = text.getBytes();
		byte[] langBytes = lang.getBytes("US-ASCII");
		int langLength = langBytes.length;
		int textLength = textBytes.length;
		byte[] payload = new byte[1 + langLength + textLength];
		payload[0] = (byte) langLength;
		System.arraycopy(langBytes, 0, payload, 1, langLength);
		System.arraycopy(textBytes, 0, payload, 1 + langLength, textLength);
		NdefRecord record = new NdefRecord(NdefRecord.TNF_WELL_KNOWN,
				NdefRecord.RTD_TEXT, new byte[0], payload);
		NdefRecord[] records = { record };
		NdefMessage message = new NdefMessage(records);
		return message;
	}

	/**
	 * Creates a NDEF Text Record.
	 *
	 * @param text
	 *            Text to write
	 * @return NDEF Record
	 * @throws UnsupportedEncodingException
	 */
	private NdefRecord createNdefTextRecord(String text)
			throws UnsupportedEncodingException {
		if(text.length() == 0) {
			return null;
		}
		String lang = "en";
		byte[] textBytes = text.getBytes();
		byte[] langBytes = lang.getBytes("US-ASCII");
		int langLength = langBytes.length;
		int textLength = textBytes.length;
		byte[] payload = new byte[1 + langLength + textLength];
		payload[0] = (byte) langLength;
		System.arraycopy(langBytes, 0, payload, 1, langLength);
		System.arraycopy(textBytes, 0, payload, 1 + langLength, textLength);
		NdefRecord record = new NdefRecord(NdefRecord.TNF_WELL_KNOWN,
				NdefRecord.RTD_TEXT, new byte[0], payload);
		return record;
	}


	/**
	 * Creates a NDEF Uri Message.
	 * 
	 * @param uri
	 *            Uri to write
	 * @return NDEF Message
	 */
	private NdefMessage createNdefUriMessage(String uri) {
		if(uri.length() == 0
	    || uri.endsWith("//")
		|| uri.endsWith(":")
		|| uri.endsWith(".")) {
			return null;
		}
		NdefRecord record = NdefRecord.createUri(uri);
		NdefRecord[] records = { record };
		NdefMessage message = new NdefMessage(records);
		return message;
	}

	/**
	 * Creates a Bluetooth Secure Simple Pairing Message.
	 *
	 * @param mac
	 *            Bluetooth MAC Address to Write
	 * @return NDEF Message
	 */
// MGN	private NdefMessage createNdefBSSPMessage() {
// MGN		byte[] payloadHs = { 0x12, (byte) 0xD1, 0x02, 0x04, 0x61, 0x63, 0x01, 0x01, 0x30, 0x00 };
// MGN		NdefRecord recordHs = new NdefRecord(NdefRecord.TNF_WELL_KNOWN,
// MGN				NdefRecord.RTD_HANDOVER_SELECT, new byte[0], payloadHs);

// MGN		byte[] deviceMAC = hexStringToByteArray(NdefFragment.getBtMac().replaceAll(":", ""));
// MGN		byte[] deviceName = NdefFragment.getBtName().getBytes();
// MGN		byte[] deviceClass = hexStringToByteArray(NdefFragment.getBtClass().replaceAll(":", ""));

// MGN		if(deviceMAC.length != 6
// MGN		|| deviceName.length == 0
// MGN		|| deviceClass.length != 3) {
// MGN			return null;
// MGN		}
// MGN		byte[] payloadBt = new byte[deviceMAC.length + deviceName.length
// MGN				 + deviceClass.length + 2 + 4];

		// Payload Size
// MGN		payloadBt[0] = (byte) payloadBt.length;

// MGN		System.arraycopy(deviceMAC, 0, payloadBt, 2, deviceMAC.length);
// MGN		payloadBt[8] = (byte) (deviceName.length + 1);
// MGN		payloadBt[9] = 0x09; // Device Name identifier
// MGN		System.arraycopy(deviceName, 0, payloadBt, 10, deviceName.length);
// MGN		payloadBt[8 + deviceName.length + 2] = (byte) (deviceClass.length + 1);
// MGN		payloadBt[8 + deviceName.length + 3] = 0x0D; // Service Name identifier
// MGN		System.arraycopy(deviceClass, 0, payloadBt, 8 + deviceName.length + 4,
// MGN				deviceClass.length);
// MGN		NdefRecord recordBt = new NdefRecord(NdefRecord.TNF_MIME_MEDIA,
// MGN				"application/vnd.bluetooth.ep.oob".getBytes(), new byte[0],
// MGN				payloadBt);
// MGN		NdefRecord[] records = { recordHs, recordBt };
// MGN		NdefMessage message = new NdefMessage(records);
// MGN		return message;
// MGN	}
	
	/**
	 * Creates a NDEF SmartPoster Message.
	 * 
	 * @param uri
	 *            Uri to write
	 * @param title
	 *            Text to write
	 * @return NDEF Message
	 * @throws UnsupportedEncodingException 
	 */
	private NdefMessage createNdefSpMessage(String title, String uri) throws UnsupportedEncodingException {
		if(title.length() == 0)
			return null;
		
		if(uri.length() == 0
		|| uri.endsWith("//")
		|| uri.endsWith(":")
		|| uri.endsWith(".")) {
			return null;
		}
		
		String lang = "en";
		byte[] textBytes = title.getBytes();
		byte[] langBytes = lang.getBytes("US-ASCII");
		int langLength = langBytes.length;
		int textLength = textBytes.length;
		byte[] payload = new byte[1 + langLength + textLength];
		payload[0] = (byte) langLength;
		System.arraycopy(langBytes, 0, payload, 1, langLength);
		System.arraycopy(textBytes, 0, payload, 1 + langLength, textLength);

		NdefRecord recordTitle = new NdefRecord(NdefRecord.TNF_WELL_KNOWN,
				NdefRecord.RTD_TEXT, new byte[0], payload);
		NdefRecord recordLink = NdefRecord.createUri(uri);

		NdefRecord[] records = new NdefRecord[] { recordTitle, recordLink };
		NdefMessage messageSp = new NdefMessage(records);
		
		byte[] bytes = messageSp.toByteArray();
		NdefRecord recordSp = new NdefRecord((short) 0x01,
				NdefRecord.RTD_SMART_POSTER, null, bytes);

		return new NdefMessage( new NdefRecord[] { recordSp });
	}

	public static byte[] hexStringToByteArray(String s) {
		int len = s.length();
		byte[] data = new byte[len / 2];
		for (int i = 0, j = (len - 2) / 2; i < len; i += 2, j--) {
			data[j] = (byte) ((Character.digit(s.charAt(i), 16) << 4) + Character
					.digit(s.charAt(i + 1), 16));
		}
		return data;
	}

	/**
	 * Closes the Reader, Writes the NDEF Message via Android function and
	 * reconnects the Reader.
	 * 
	 * @return NDEF Writing time
	 * @throws IOException
	 * @throws FormatException
	 * @throws CommandNotSupportedException 
	 */
	private long NDEFWrite(NdefMessage msg) throws IOException, FormatException, CommandNotSupportedException {
		// Time statistics to return
		long timeNdefWrite = 0;
		long RegTimeOutStart = System.currentTimeMillis();
		
		// Write the NDEF using NfcA commands to avoid problems when dealing with protected tags
		// Calling reader close / connect resets the authenticated status
		reader.writeNDEF(msg, null);

		timeNdefWrite = System.currentTimeMillis() - RegTimeOutStart;
		
		// Return the calculated time value
		return timeNdefWrite;
	}





	/**
	 * Rounds the voltage to one single decimal.
	 */
	public double round(double value) {
		return Math.rint(value * 10) / 10;
	}

	/**
	 * Helper function to show messages on the screen.
	 * 
	 * @param temp Message
	 */
	protected void showResultDialogMessage(String temp) {
		new AlertDialog.Builder(main).setMessage(temp)
				.setPositiveButton("OK", new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
					}
				}).show();
	}

	/**
	 * Creates a NDEF Message
	 * 
	 * @param text
	 *            Text to write
	 * @return NDEF Message
	 * @throws UnsupportedEncodingException
	 */
	private NdefMessage createNdefMessage(String text)
			throws UnsupportedEncodingException {
		String lang = "en";
		byte[] textBytes = text.getBytes();
		byte[] langBytes = lang.getBytes("US-ASCII");
		int langLength = langBytes.length;
		int textLength = textBytes.length;
		byte[] payload = new byte[1 + langLength + textLength];
		payload[0] = (byte) langLength;
		System.arraycopy(langBytes, 0, payload, 1, langLength);
		System.arraycopy(textBytes, 0, payload, 1 + langLength, textLength);

		NdefRecord record = new NdefRecord(NdefRecord.TNF_WELL_KNOWN,
				NdefRecord.RTD_TEXT, new byte[0], payload);
		NdefRecord[] records = { record };
		NdefMessage message = new NdefMessage(records);
		return message;
	}

	protected byte[] concat(byte[] one, byte[] two) {
		if (one == null) {
			one = new byte[0];
		}
		if (two == null) {
			two = new byte[0];
		}
		byte[] combined = new byte[one.length + two.length];
		System.arraycopy(one, 0, combined, 0, one.length);
		System.arraycopy(two, 0, combined, one.length, two.length);
		return combined;
	}

	@Override
	public void onWriteEEPROM(final int bytes) {
		main.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				//SpeedTestFragment.setDatarateCallback(String.valueOf(bytes) + " Bytes written");
			}
		});
	}
}
