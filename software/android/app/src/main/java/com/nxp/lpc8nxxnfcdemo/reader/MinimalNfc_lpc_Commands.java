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
import java.util.Arrays;

import android.nfc.FormatException;
import android.nfc.NdefMessage;
import android.nfc.Tag;
import android.nfc.tech.MifareUltralight;

import com.nxp.lpc8nxxnfcdemo.exceptions.CommandNotSupportedException;
import com.nxp.lpc8nxxnfcdemo.listeners.WriteEEPROMListener;
import com.nxp.lpc8nxxnfcdemo.reader.Nfc_Get_Version.Prod;

/**
 * Class specific for the functions of The NTAG I2C.
 *
 * @author NXP67729
 *
 */
public class MinimalNfc_lpc_Commands extends Lpc_Enabled_Commands {

	private final int firstSectorMemsize = (0xFF - 0x4) * 4;
	private MifareUltralight mfu;
	private Prod tagType;
	private byte[] answer;
	private static int waitTime = 20;
	public boolean flag = false;
	/**
	 * Special Registers of the NTAG I2C.
	 *
	 */
	public enum Register {
		Session((byte) 0xF8), Configuration((byte) 0xE8), SRAM_Begin(
				(byte) 0xF0), User_memory_Begin((byte) 0x06), UID((byte) 0x00), Proprietary_memory_Begin((byte)0x04);

		private byte value;
		private Register(byte value) {
			this.value = value;
		}
		public byte getValue() {
			return value;
		}
	}

	// ---------------------------------------------------------------------------------
	// Begin Public Functions
	// ---------------------------------------------------------------------------------

	/**
	 * Constructor.
	 *
	 * @param tag
	 *            Tag to connect
	 * @throws IOException
	 */
	public MinimalNfc_lpc_Commands(Tag tag, Prod prod) throws IOException {
		tagType = prod;
		blockSize = 4;
		SRAMSize = 64;
		this.mfu = MifareUltralight.get(tag);
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#close()
	 */
	@Override
	public void close() throws IOException {
		mfu.close();
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#connect()
	 */
	@Override
	public void connect() throws IOException {
		mfu.connect();
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#isConnected()
	 */
	@Override
	public boolean isConnected() {
		return mfu.isConnected();
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#getLastAnswer()
	 */
	@Override
	public byte[] getLastAnswer() {
		return answer;
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#getProduct()
	 */
	@Override
	public Prod getProduct() throws IOException {
		// returns generic NTAG_I2C_1k, because getVersion is not possible
		return tagType;
	}


	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#writeEEPROM(byte[])
	 */
	@Override
	public void writeEEPROM(byte[] data, WriteEEPROMListener listener) throws IOException, FormatException,
			CommandNotSupportedException {
		if ((tagType == Prod.NTAG_I2C_2k || tagType == Prod.NTAG_I2C_2k_Plus)
				&& data.length > firstSectorMemsize) {
			throw new CommandNotSupportedException(
					"writeEEPROM is not Supported for this Phone, with Data bigger then First Sector("
							+ firstSectorMemsize + " Bytes)");
		}
		
		if (data.length > getProduct().getMemsize()) {
			throw new IOException("Data is too long");
		}

		byte[] temp;
		int blockNr = Register.User_memory_Begin.getValue();

		// write till all Data is written
		for (int i = 0; i < data.length; i += 4) {
			temp = Arrays.copyOfRange(data, i, i + 4);
			mfu.writePage(blockNr, temp);
			blockNr++;
			
			// Inform the listener about the writing
			if(listener != null) {
				listener.onWriteEEPROM(i + 4);
			}
		}
	}

	@Override
	public void writeEEPROMCustom(byte[] data, int startAddr,WriteEEPROMListener listener,boolean dummy) throws IOException, FormatException,
			CommandNotSupportedException {
		if ((tagType == Prod.NTAG_I2C_2k || tagType == Prod.NTAG_I2C_2k_Plus)
				&& data.length > firstSectorMemsize) {
			throw new CommandNotSupportedException(
					"writeEEPROM is not Supported for this Phone, with Data bigger then First Sector("
							+ firstSectorMemsize + " Bytes)");
		}

		if (data.length > getProduct().getMemsize()) {
			throw new IOException("Data is too long");
		}

		byte[] temp;
		int blockNr = (byte) (startAddr & 0xFF);

		// write till all Data is written
		for (int i = 0; i < data.length; i += 4) {
			temp = Arrays.copyOfRange(data, i, i + 4);
			mfu.writePage(blockNr, temp);
			blockNr++;

			// Inform the listener about the writing
			if(listener != null) {
				listener.onWriteEEPROM(i + 4);
			}
		}
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#writeEEPROM(int,
	 * byte[])
	 */
	@Override
	public void writeEEPROM(int startAddr, byte[] data) throws IOException,
			FormatException {
		// Nothing will be done for now
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#readEEPROM(int, int)
	 */
	@Override
	public byte[] readEEPROM(int absStart, int absEnd) throws IOException,
			FormatException, CommandNotSupportedException {

		if ((tagType == Prod.NTAG_I2C_2k && absEnd > 0xFF)
				|| tagType == Prod.NTAG_I2C_2k_Plus && absEnd > 0xE1)
			throw new CommandNotSupportedException(
					"readEEPROM is not Supported for this Phone on Second Sector");

		byte[] temp = new byte[0];
		answer = new byte[0];

		if (absStart > 0xFF) {
			absStart = 0xFF;
		}

		if (absEnd > 0xFF) {
			absEnd = 0xFF;
		}

		int i;
		for (i = absStart; i <= (absEnd - 3); i += 4) {
			temp = mfu.readPages(i);
			answer = concat(answer, temp);
		}

		if (i < absEnd) {
			temp = mfu.readPages(absEnd - 3);
			byte[] bla = Arrays.copyOfRange(temp, (i - (absEnd - 3)) * 4, 16);
			answer = concat(answer, bla);
		}
		return answer;
	}



	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#writeEmptyNdef()
	 */
	@Override
	public void writeEmptyNdef() throws IOException, FormatException {
		// Nothing done for now
	}
	
	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#writeDefaultNdef()
	 */
	@Override
	public void writeDefaultNdef() throws IOException, FormatException {
		byte[] data = new byte[4];

		data[0] = (byte) 0x03;
		data[1] = (byte) 0x60;
		data[2] = (byte) 0x91;
		data[3] = (byte) 0x02;
		
		mfu.writePage((byte) 0x04, data);
		
		data[0] = (byte) 0x35;
		data[1] = (byte) 0x53;
		data[2] = (byte) 0x70;
		data[3] = (byte) 0x91;
		
		mfu.writePage((byte) 0x05, data);
		
		data[0] = (byte) 0x01;
		data[1] = (byte) 0x14;
		data[2] = (byte) 0x54;
		data[3] = (byte) 0x02;
		
		mfu.writePage((byte) 0x06, data);
		
		data[0] = (byte) 0x65;
		data[1] = (byte) 0x6E;
		data[2] = (byte) 0x4E;
		data[3] = (byte) 0x54;
		
		mfu.writePage((byte) 0x07, data);
		
		data[0] = (byte) 0x41;
		data[1] = (byte) 0x47;
		data[2] = (byte) 0x20;
		data[3] = (byte) 0x49;
		
		mfu.writePage((byte) 0x08, data);

		data[0] = (byte) 0x32;
		data[1] = (byte) 0x43;
		data[2] = (byte) 0x20;
		data[3] = (byte) 0x45;
		
		mfu.writePage((byte) 0x09, data);

		data[0] = (byte) 0x58;
		data[1] = (byte) 0x50;
		data[2] = (byte) 0x4C;
		data[3] = (byte) 0x4F;
		
		mfu.writePage((byte) 0x0A, data);
		
		data[0] = (byte) 0x52;
		data[1] = (byte) 0x45;
		data[2] = (byte) 0x52;
		data[3] = (byte) 0x51;
		
		mfu.writePage((byte) 0x0B, data);
		
		data[0] = (byte) 0x01;
		data[1] = (byte) 0x19;
		data[2] = (byte) 0x55;
		data[3] = (byte) 0x01;
		
		mfu.writePage((byte) 0x0C, data);
		
		data[0] = (byte) 0x6E;
		data[1] = (byte) 0x78;
		data[2] = (byte) 0x70;
		data[3] = (byte) 0x2E;
		
		mfu.writePage((byte) 0x0D, data);
		
		data[0] = (byte) 0x63;
		data[1] = (byte) 0x6F;
		data[2] = (byte) 0x6D;
		data[3] = (byte) 0x2F;
		
		mfu.writePage((byte) 0x0E, data);

		data[0] = (byte) 0x64;
		data[1] = (byte) 0x65;
		data[2] = (byte) 0x6D;
		data[3] = (byte) 0x6F;
		
		mfu.writePage((byte) 0x0F, data);
		
		data[0] = (byte) 0x62;
		data[1] = (byte) 0x6F;
		data[2] = (byte) 0x61;
		data[3] = (byte) 0x72;
		
		mfu.writePage((byte) 0x10, data);
		
		data[0] = (byte) 0x64;
		data[1] = (byte) 0x2F;
		data[2] = (byte) 0x4F;
		data[3] = (byte) 0x4D;
		
		mfu.writePage((byte) 0x11, data);
		
		data[0] = (byte) 0x35;
		data[1] = (byte) 0x35;
		data[2] = (byte) 0x36;
		data[3] = (byte) 0x39;
		
		mfu.writePage((byte) 0x12, data);
		
		data[0] = (byte) 0x54;
		data[1] = (byte) 0x0F;
		data[2] = (byte) 0x14;
		data[3] = (byte) 0x61;
		
		mfu.writePage((byte) 0x13, data);
		
		data[0] = (byte) 0x6E;
		data[1] = (byte) 0x64;
		data[2] = (byte) 0x72;
		data[3] = (byte) 0x6F;
		
		mfu.writePage((byte) 0x14, data);

		data[0] = (byte) 0x69;
		data[1] = (byte) 0x64;
		data[2] = (byte) 0x2E;
		data[3] = (byte) 0x63;
		
		mfu.writePage((byte) 0x15, data);
		
		data[0] = (byte) 0x6F;
		data[1] = (byte) 0x6D;
		data[2] = (byte) 0x3A;
		data[3] = (byte) 0x70;
		
		mfu.writePage((byte) 0x16, data);

		data[0] = (byte) 0x6B;
		data[1] = (byte) 0x67;
		data[2] = (byte) 0x63;
		data[3] = (byte) 0x6F;
		
		mfu.writePage((byte) 0x17, data);
		
		data[0] = (byte) 0x6D;
		data[1] = (byte) 0x2E;
		data[2] = (byte) 0x6E;
		data[3] = (byte) 0x78;
		
		mfu.writePage((byte) 0x18, data);

		data[0] = (byte) 0x70;
		data[1] = (byte) 0x2E;
		data[2] = (byte) 0x6E;
		data[3] = (byte) 0x74;
		
		mfu.writePage((byte) 0x19, data);
		
		data[0] = (byte) 0x61;
		data[1] = (byte) 0x67;
		data[2] = (byte) 0x69;
		data[3] = (byte) 0x32;
		
		mfu.writePage((byte) 0x1A, data);
		
		data[0] = (byte) 0x63;
		data[1] = (byte) 0x64;
		data[2] = (byte) 0x65;
		data[3] = (byte) 0x6D;
		
		mfu.writePage((byte) 0x1B, data);
		
		data[0] = (byte) 0x6F;
		data[1] = (byte) 0x5F;
		data[2] = (byte) 0xFE;
		data[3] = (byte) 0x00;
		
		mfu.writePage((byte) 0x1C, data);
	}




	/*
	 * (non-Javadoc)
	 * 
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#writeNDEF()
	 */
	@Override
	public void writeNDEF(NdefMessage message, WriteEEPROMListener listener) throws IOException,
			FormatException, CommandNotSupportedException {
		byte[] ndefMessageByte = createRawNdefTlv(message);
		writeEEPROM(ndefMessageByte, listener);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.nxp.nfc_demo.reader.Lpc_Enabled_Commands#readNDEF()
	 */
	@Override
	public NdefMessage readNDEF() throws IOException, FormatException, CommandNotSupportedException {
		int ndefsize = 0;
		int tlvsize = 0;
		int tlvPlusNdef = 0;

		// get TLV
		byte[] tlv = readEEPROM(Register.User_memory_Begin.getValue(),
				Register.User_memory_Begin.getValue() + 3);

		// checking TLV - maybe there are other TLVs on the tag
		if (tlv[0] != 0x03) {
			throw new FormatException("Format on Tag not supported");
		}

		if (tlv[1] != (byte) 0xFF) {
			ndefsize = (tlv[1] & 0xFF);
			tlvsize = 2;
			tlvPlusNdef = tlvsize + ndefsize;
		} else {
			ndefsize = (tlv[3] & 0xFF);
			ndefsize |= ((tlv[2] << 8) & 0xFF00);
			tlvsize = 4;
			tlvPlusNdef = tlvsize + ndefsize;
		}

			// Read NDEF Message
			byte[] data = readEEPROM(Register.User_memory_Begin.getValue(),
					Register.User_memory_Begin.getValue() + (tlvPlusNdef / 4) + 3);

			// delete TLV
			data = Arrays.copyOfRange(data, tlvsize, data.length);
			// delete end of String which is not part of the NDEF Message
			data = Arrays.copyOf(data, ndefsize);

			// Interpret Bytes
			NdefMessage message = new NdefMessage(data);
			return message;

	}

	// -------------------------------------------------------------------
	// Helping function
	// -------------------------------------------------------------------

	/**
	 * create a Raw NDEF TLV from a NDEF Message
	 *
	 * @param ndefMessage
	 *            NDEF Message to put in the NDEF TLV
	 * @return Byte Array of NDEF Message
	 * @throws UnsupportedEncodingException
	 */
	private byte[] createRawNdefTlv(NdefMessage ndefMessage)
			throws UnsupportedEncodingException {
		// creating NDEF
		byte[] ndefMessageByte = ndefMessage.toByteArray();
		int ndefMessageSize = ndefMessageByte.length;
		byte[] message;

		if (ndefMessageSize < 0xFF) {
			message = new byte[ndefMessageSize + 3];
			byte tlvSize = 0;
			tlvSize = (byte) ndefMessageSize;
			message[0] = (byte) 0x03;
			message[1] = (byte) tlvSize;
			message[message.length - 1] = (byte) 0xFE;
			System.arraycopy(ndefMessageByte, 0, message, 2,
					ndefMessageByte.length);
		} else {
			message = new byte[ndefMessageSize + 5];
			int tlvSize = ndefMessageSize;
			tlvSize |= 0xFF0000;
			message[0] = (byte) 0x03;
			message[1] = (byte) ((tlvSize >> 16) & 0xFF);
			message[2] = (byte) ((tlvSize >> 8) & 0xFF);
			message[3] = (byte) (tlvSize & 0xFF);
			message[message.length - 1] = (byte) 0xFE;
			System.arraycopy(ndefMessageByte, 0, message, 4,
					ndefMessageByte.length);
		}
		return message;
	}



}
