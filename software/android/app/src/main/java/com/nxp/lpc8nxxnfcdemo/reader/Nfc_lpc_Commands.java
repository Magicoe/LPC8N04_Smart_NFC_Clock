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

import com.nxp.lpc8nxxnfcdemo.exceptions.CommandNotSupportedException;
import com.nxp.lpc8nxxnfcdemo.listeners.WriteEEPROMListener;
import com.nxp.lpc8nxxnfcdemo.reader.Nfc_Get_Version.Prod;

/**
 * Class specific for the functions of The NTAG I2C.
 * 
 * @author NXP67729
 * 
 */
public class Nfc_lpc_Commands extends Lpc_Enabled_Commands {
	private static final int DEFAULT_NDEF_MESSAGE_SIZE = 0;
	private static final int EMPTY_NDEF_MESSAGE_SIZE = 104;
	private static final int SRAM_SIZE = 64;
	private static final int SRAM_BLOCK_SIZE = 4;

	private Nfc_Commands reader;
	private Tag tag;
	private byte[] answer;
	private Nfc_Get_Version getVersionResponse;
	private byte sramSector;
	private boolean TimeOut = false;
	private Object lock = new Object();

	/**
	 * Special Registers of the NTAG I2C.
	 *
	 */
	public enum Register {
		Session((byte) 0xF8), Session_PLUS((byte) 0xEC), Configuration((byte) 0xE8), SRAM_Begin((byte) 0xF0), 
		Capability_Container((byte) 0x03), User_memory_Begin((byte) 0x04), UID((byte) 0x00),
		AUTH0((byte) 0xE3), ACCESS((byte) 0xE4), PWD((byte) 0xE5), PACK((byte) 0xE6), PT_I2C((byte) 0xE7);
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
	public Nfc_lpc_Commands(Tag tag) throws IOException {
		blockSize = SRAM_BLOCK_SIZE;
		SRAMSize = SRAM_SIZE;
		this.reader = new Nfc_Commands(tag);
		this.tag = tag;
		connect();
		if (getProduct() == Prod.NTAG_I2C_2k) {
			sramSector = 1;
		} else {
			sramSector = 0;
		}
		close();

	}

	/*
	 * (non-Javadoc).
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#close()
	 */
	@Override
	public void close() throws IOException {
		reader.close();
	}

	/*
	 * (non-Javadoc).
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#connect()
	 */
	@Override
	public void connect() throws IOException {
		reader.connect();
	}

	/*
	 * (non-Javadoc).
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#isConnected()
	 */
	@Override
	public boolean isConnected() {
		return reader.isConnected();
	}

	/*
	 * (non-Javadoc).
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#getLastAnswer()
	 */
	@Override
	public byte[] getLastAnswer() {
		return answer;
	}

	/*
	 * (non-Javadoc).
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#getProduct()
	 */
	@Override
	public Prod getProduct() throws IOException {
		if (getVersionResponse == null) {
			try {
				getVersionResponse = new Nfc_Get_Version(reader.getVersion());
			} catch (Exception e) {
				e.printStackTrace();
				try {
					reader.close();
					reader.connect();
					byte[] temp = reader.read((byte) 0x00);

					if (temp[0] == (byte) 0x04 && temp[12] == (byte) 0xE1
							&& temp[13] == (byte) 0x10
							&& temp[14] == (byte) 0x6D
							&& temp[15] == (byte) 0x00) {

						temp = reader.read((byte) 0xE8);
						getVersionResponse = Nfc_Get_Version.NTAG_I2C_1k;

					} else if (temp[0] == (byte) 0x04
							&& temp[12] == (byte) 0xE1
							&& temp[13] == (byte) 0x10
							&& temp[14] == (byte) 0xEA
							&& temp[15] == (byte) 0x00) {
						getVersionResponse = Nfc_Get_Version.NTAG_I2C_2k;
					}
				} catch (FormatException e2) {
					reader.close();
					reader.connect();
					e2.printStackTrace();
					getVersionResponse = Nfc_Get_Version.NTAG_I2C_1k;
				}

			}
		}

		return getVersionResponse.Get_Product();
	}


	@Override
	public void writeEEPROMCustom(byte[] data, int startAddr,WriteEEPROMListener listener,boolean dummy) throws IOException, FormatException,
			CommandNotSupportedException {
		// Nothing
	}
	/*
	 * (non-Javadoc)
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#writeEEPROM(byte[])
	 */
	@Override
	public void writeEEPROM(byte[] data, WriteEEPROMListener listener) throws IOException, FormatException {
		if (data.length > getProduct().getMemsize()) {
			throw new IOException("Data is to long");
		}
		reader.SectorSelect((byte) 0);
		byte[] temp;
		int index = 0;
		byte blockNr = Register.User_memory_Begin.getValue();

		// write till all Data is written or the Block 0xFF was written(BlockNr
		// should be 0 then, because of the type byte)
		for (index = 0; index < data.length && blockNr != 0; index += 4) {
			// NTAG I2C Plus sits the Config registers in Sector 0
			if(getProduct() == Prod.NTAG_I2C_2k_Plus && blockNr == (byte) 0xE2) {
				break;
			}

			temp = Arrays.copyOfRange(data, index, index + 4);
			reader.write(temp, blockNr);
			blockNr++;
			
			// Inform the listener about the writing
			if(listener != null) {
				listener.onWriteEEPROM(index + 4);
			}
		}

		// If Data is left write to the 1. Sector
		if (index < data.length) {
			reader.SectorSelect((byte) 1);
			blockNr = 0;
			for (; index < data.length; index += 4) {
				temp = Arrays.copyOfRange(data, index, index + 4);
				reader.write(temp, blockNr);
				blockNr++;
				
				// Inform the listener about the writing
				if(listener != null) {
					listener.onWriteEEPROM(index + 4);
				}
			}
		}
	}


	/*
	 * (non-Javadoc)
	 *
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#writeEEPROM(int,
	 * byte[])
	 */
	@Override
	public void writeEEPROM(int startAddr, byte[] data) throws IOException,
			FormatException {

		if ((startAddr & 0x100) != 0x000 && (startAddr & 0x200) != 0x100) {
			throw new FormatException("Sector not supported");
		}
		reader.SectorSelect((byte) ((startAddr & 0x200) >> 16));
		byte[] temp;
		int index = 0;
		byte blockNr = (byte) (startAddr & 0xFF);

		// write till all Data is written or the Block 0xFF was written(BlockNr
		// should be
		// 0 then, because of the type byte)
		for (index = 0; index < data.length && blockNr != 0; index += 4) {
			temp = Arrays.copyOfRange(data, index, index + 4);
			reader.write(temp, blockNr);
			blockNr++;
		}

		// If Data is left write and the first Sector was not already written
		// switch to the first
		if (index < data.length && (startAddr & 0x100) != 0x100) {
			reader.SectorSelect((byte) 1);
			blockNr = 0;
			for (; index < data.length; index += 4) {
				temp = Arrays.copyOfRange(data, index, index + 4);
				reader.write(temp, blockNr);
				blockNr++;
			}
		} else if ((startAddr & 0x100) == 0x100) {
			throw new IOException("Data is to long");
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#readEEPROM(int, int)
	 */
	@Override
	public byte[] readEEPROM(int absStart, int absEnd) throws IOException,
			FormatException {
		int maxfetchsize = reader.getMaxTransceiveLength();
		int maxFastRead = (maxfetchsize - 2) / 4;
		int fetchStart = absStart;
		int fetchEnd = 0;
		byte[] data = null;
		byte[] temp = null;

		reader.SectorSelect((byte) 0);

		while (fetchStart <= absEnd) {
			fetchEnd = fetchStart + maxFastRead - 1;
			// check for last read, fetch only rest
			if (fetchEnd > absEnd) {
				fetchEnd = absEnd;
			}

			// check for sector change in between and reduce fast_read to stay within sector
			if (getProduct() != Prod.NTAG_I2C_2k_Plus) {
				if ((fetchStart & 0xFF00) != (fetchEnd & 0xFF00)) {
					fetchEnd = (fetchStart & 0xFF00) + 0xFF;
				}
			} else {
				if ((fetchStart & 0xFF00) == 0 && (fetchEnd > 0xE2)) {
					fetchEnd = (fetchStart & 0xFF00) + 0xE1;
				}
			}
			temp = reader.fast_read((byte) (fetchStart & 0x00FF), (byte) (fetchEnd & 0x00FF));
			data = concat(data, temp);
			
			// calculate next fetch_start
			fetchStart = fetchEnd + 1;

			// check for sector change in between and reduce fast_read to stay within sector
			if (getProduct() != Prod.NTAG_I2C_2k_Plus) {
				if ((fetchStart & 0xFF00) != (fetchEnd & 0xFF00)) {
					reader.SectorSelect((byte) 1);
				}
			} else {
				if ((fetchStart & 0xFF00) == 0 && (fetchEnd >= 0xE1)) {
					reader.SectorSelect((byte) 1);
					fetchStart = 0x100;

					// Update the absEnd with pages not read on Sector 0
					absEnd = absEnd + (0xFF - 0xE2);
				}
			}
		}
		// Let's go back to Sector 0
		reader.SectorSelect((byte) 0);
		return data;
	}



	/*
	 * (non-Javadoc)
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#writeEmptyNdef()
	 */
	@Override
	public void writeEmptyNdef() throws IOException, FormatException {
		int index = 0;
		byte[] data = new byte[4];
		index = 0;

		reader.SectorSelect((byte) 0);

		data[index++] = (byte) 0x03;
		data[index++] = (byte) 0x00;
		data[index++] = (byte) 0xFE;
		data[index++] = (byte) 0x00;

		reader.write(data, (byte) 0x04);
	}
	
	/*
	 * (non-Javadoc)
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#writeEmptyNdef()
	 */
	@Override
	public void writeDefaultNdef() throws IOException, FormatException {
		byte[] data = new byte[4];

		reader.SectorSelect((byte) 0);

		data[0] = (byte) 0x03;
		data[1] = (byte) 0x60;
		data[2] = (byte) 0x91;
		data[3] = (byte) 0x02;
		
		reader.write(data, (byte) 0x04);
		
		data[0] = (byte) 0x35;
		data[1] = (byte) 0x53;
		data[2] = (byte) 0x70;
		data[3] = (byte) 0x91;
		
		reader.write(data, (byte) 0x05);
		
		data[0] = (byte) 0x01;
		data[1] = (byte) 0x14;
		data[2] = (byte) 0x54;
		data[3] = (byte) 0x02;
		
		reader.write(data, (byte) 0x06);
		
		data[0] = (byte) 0x65;
		data[1] = (byte) 0x6E;
		data[2] = (byte) 0x4E;
		data[3] = (byte) 0x54;
		
		reader.write(data, (byte) 0x07);
		
		data[0] = (byte) 0x41;
		data[1] = (byte) 0x47;
		data[2] = (byte) 0x20;
		data[3] = (byte) 0x49;
		
		reader.write(data, (byte) 0x08);

		data[0] = (byte) 0x32;
		data[1] = (byte) 0x43;
		data[2] = (byte) 0x20;
		data[3] = (byte) 0x45;
		
		reader.write(data, (byte) 0x09);

		data[0] = (byte) 0x58;
		data[1] = (byte) 0x50;
		data[2] = (byte) 0x4C;
		data[3] = (byte) 0x4F;
		
		reader.write(data, (byte) 0x0A);
		
		data[0] = (byte) 0x52;
		data[1] = (byte) 0x45;
		data[2] = (byte) 0x52;
		data[3] = (byte) 0x51;
		
		reader.write(data, (byte) 0x0B);
		
		data[0] = (byte) 0x01;
		data[1] = (byte) 0x19;
		data[2] = (byte) 0x55;
		data[3] = (byte) 0x01;
		
		reader.write(data, (byte) 0x0C);
		
		data[0] = (byte) 0x6E;
		data[1] = (byte) 0x78;
		data[2] = (byte) 0x70;
		data[3] = (byte) 0x2E;
		
		reader.write(data, (byte) 0x0D);
		
		data[0] = (byte) 0x63;
		data[1] = (byte) 0x6F;
		data[2] = (byte) 0x6D;
		data[3] = (byte) 0x2F;
		
		reader.write(data, (byte) 0x0E);

		data[0] = (byte) 0x64;
		data[1] = (byte) 0x65;
		data[2] = (byte) 0x6D;
		data[3] = (byte) 0x6F;
		
		reader.write(data, (byte) 0x0F);
		
		data[0] = (byte) 0x62;
		data[1] = (byte) 0x6F;
		data[2] = (byte) 0x61;
		data[3] = (byte) 0x72;
		
		reader.write(data, (byte) 0x10);
		
		data[0] = (byte) 0x64;
		data[1] = (byte) 0x2F;
		data[2] = (byte) 0x4F;
		data[3] = (byte) 0x4D;
		
		reader.write(data, (byte) 0x11);
		
		data[0] = (byte) 0x35;
		data[1] = (byte) 0x35;
		data[2] = (byte) 0x36;
		data[3] = (byte) 0x39;
		
		reader.write(data, (byte) 0x12);
		
		data[0] = (byte) 0x54;
		data[1] = (byte) 0x0F;
		data[2] = (byte) 0x14;
		data[3] = (byte) 0x61;
		
		reader.write(data, (byte) 0x13);
		
		data[0] = (byte) 0x6E;
		data[1] = (byte) 0x64;
		data[2] = (byte) 0x72;
		data[3] = (byte) 0x6F;
		
		reader.write(data, (byte) 0x14);

		data[0] = (byte) 0x69;
		data[1] = (byte) 0x64;
		data[2] = (byte) 0x2E;
		data[3] = (byte) 0x63;
		
		reader.write(data, (byte) 0x15);
		
		data[0] = (byte) 0x6F;
		data[1] = (byte) 0x6D;
		data[2] = (byte) 0x3A;
		data[3]= (byte) 0x70;
		
		reader.write(data, (byte) 0x16);

		data[0] = (byte) 0x6B;
		data[1] = (byte) 0x67;
		data[2] = (byte) 0x63;
		data[3] = (byte) 0x6F;
		
		reader.write(data, (byte) 0x17);
		
		data[0] = (byte) 0x6D;
		data[1] = (byte) 0x2E;
		data[2] = (byte) 0x6E;
		data[3] = (byte) 0x78;
		
		reader.write(data, (byte) 0x18);

		data[0] = (byte) 0x70;
		data[1] = (byte) 0x2E;
		data[2] = (byte) 0x6E;
		data[3] = (byte) 0x74;
		
		reader.write(data, (byte) 0x19);
		
		data[0] = (byte) 0x61;
		data[1] = (byte) 0x67;
		data[2] = (byte) 0x69;
		data[3] = (byte) 0x32;
		
		reader.write(data, (byte) 0x1A);
		
		data[0] = (byte) 0x63;
		data[1] = (byte) 0x64;
		data[2] = (byte) 0x65;
		data[3] = (byte) 0x6D;
		
		reader.write(data, (byte) 0x1B);
		
		data[0] = (byte) 0x6F;
		data[1] = (byte) 0x5F;
		data[2] = (byte) 0xFE;
		data[3] = (byte) 0x00;
		
		reader.write(data, (byte) 0x1C);
	}


	/*
	 * (non-Javadoc)
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#writeNDEF()
	 */
	@Override
	public void writeNDEF(NdefMessage message, WriteEEPROMListener listener) throws IOException,
			FormatException {
		byte[] Ndef_message_byte = createRawNdefTlv(message);
		writeEEPROM(Ndef_message_byte, listener);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.nxp.lpc8nxxnfcdemo.reader.Lpc_Enabled_Commands#readNDEF()
	 */
	@Override
	public NdefMessage readNDEF() throws IOException, FormatException {
		int ndefsize;
		int tlvsize;
		int tlvPlusNdef;

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

		// get the String out of the Message
		NdefMessage message = new NdefMessage(data);
		return message;
	}

	// -------------------------------------------------------------------
	// Helping function
	// -------------------------------------------------------------------

	/**
	 * create a Raw NDEF TLV from a NDEF Message.
	 * 
	 * @param NdefMessage
	 *            NDEF Message to put in the NDEF TLV
	 * @return Byte Array of NDEF Message
	 * @throws UnsupportedEncodingException
	 */
	private byte[] createRawNdefTlv(NdefMessage NDEFmessage)
			throws UnsupportedEncodingException {
		// creating NDEF
		byte[] ndefMessageByte = NDEFmessage.toByteArray();
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
			int TLV_size = ndefMessageSize;
			TLV_size |= 0xFF0000;
			message[0] = (byte) 0x03;
			message[1] = (byte) ((TLV_size >> 16) & 0xFF);
			message[2] = (byte) ((TLV_size >> 8) & 0xFF);
			message[3] = (byte) (TLV_size & 0xFF);
			message[message.length - 1] = (byte) 0xFE;
			System.arraycopy(ndefMessageByte, 0, message, 4,
					ndefMessageByte.length);
		}
		return message;
	}


}
