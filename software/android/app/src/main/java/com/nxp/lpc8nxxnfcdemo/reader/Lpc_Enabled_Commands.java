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

import android.nfc.FormatException;
import android.nfc.NdefMessage;
import android.nfc.Tag;
import android.nfc.tech.MifareUltralight;
import android.nfc.tech.NfcA;

import com.nxp.lpc8nxxnfcdemo.exceptions.CommandNotSupportedException;
import com.nxp.lpc8nxxnfcdemo.listeners.WriteEEPROMListener;
import com.nxp.lpc8nxxnfcdemo.reader.Nfc_Get_Version.Prod;

public abstract class Lpc_Enabled_Commands {

	/**
	 * This method returns the reader to be used based on the NFC Phone capabilities
	 * This distinction is needed because in the field we can find old NFC Phones that do not support 
	 * Sector Select or Get Version commands.
	 *
	 * @param tag
	 * @return
	 * @throws IOException
	 * @throws InterruptedException
	 */
	public static Lpc_Enabled_Commands get(Tag tag) throws IOException,
			InterruptedException {
		byte[] answer;
		byte[] command = new byte[2];
		NfcA nfca = NfcA.get(tag);
		Prod prod;

		// Check for support of setTimout to be able to send an efficient
		// sector_select - select minimal implementation if not supported
		nfca.setTimeout(20);
		// check for timeout
		if (nfca.getTimeout() < 50) {		
			// check if GetVersion is supported
			try {
				nfca.connect();
				command = new byte[1];
				command[0] = (byte) 0x60; // GET_VERSION
				answer = nfca.transceive(command);
				prod = (new Nfc_Get_Version(answer)).Get_Product();
				nfca.close();
				if (prod == Prod.NTAG_I2C_1k || prod == Prod.NTAG_I2C_2k
						|| prod == Prod.NTAG_I2C_1k_T || prod == Prod.NTAG_I2C_2k_T
						|| prod == Prod.NTAG_I2C_1k_V || prod == Prod.NTAG_I2C_2k_V
						|| prod == Prod.NTAG_I2C_1k_Plus || prod == Prod.NTAG_I2C_2k_Plus) {
					return new Nfc_lpc_Commands(tag);
				}
			} catch (Exception e) {
				e.printStackTrace();
				nfca.close();
								
				// check if sector select is supported
				try {
					nfca.connect();
					command = new byte[2];
					command[0] = (byte) 0xC2; //SECTOR_SELECT
					command[1] = (byte) 0xFF;
					answer = nfca.transceive(command);
					nfca.close();
					return new Nfc_lpc_Commands(tag);
					
				} catch (Exception e2) {
					e.printStackTrace();
					nfca.close();
				}
			}
		}

		//check if we can use the minimal Version
		MifareUltralight mfu = MifareUltralight.get(tag);
		try {
			mfu.connect();
			command = new byte[1];
			command[0] = (byte) 0x60; // GET_VERSION
			answer = mfu.transceive(command);
			prod = (new Nfc_Get_Version(answer)).Get_Product();
			mfu.close();
			if (prod == Prod.NTAG_I2C_1k || prod == Prod.NTAG_I2C_2k
					|| prod == Prod.NTAG_I2C_1k_T || prod == Prod.NTAG_I2C_2k_T
					|| prod == Prod.NTAG_I2C_1k_V || prod == Prod.NTAG_I2C_2k_V
					|| prod == Prod.NTAG_I2C_1k_Plus || prod == Prod.NTAG_I2C_2k_Plus) {
				return new MinimalNfc_lpc_Commands(tag, prod);
			}
		} catch (Exception e) {
            e.printStackTrace();
            mfu.close();
            try {
                  mfu.connect();
                  answer = mfu.readPages(0);
                  // no exception is thrown so the phone can use the mfu.readPages
                  // function
                  // also check if:
                  // - tag is from NXP (byte 0 == 0x04)
                  // - CC corresponds to a NTAG I2C 1K
                  if (answer[0] == (byte) 0x04 && answer[12] == (byte) 0xE1
                                && answer[13] == (byte) 0x10 && answer[14] == (byte) 0x6D
                                && answer[15] == (byte) 0x00) {
                	  // check if Config is readable (distinguish from NTAG216), if
                      // not exception is thrown, and tag is not an
                      // NTAG I2C 1k
                      answer = mfu.readPages(0xE8);
                      
                      // Try to read session registers to differentiate between standard and PLUS products
                	  try {	 
                		  answer = mfu.readPages(0xEC);
                		  mfu.close();

                		  for(int i = 0; i < 4; i++) {
                			  if(answer[i] != 0x00) {
                				  prod = Prod.NTAG_I2C_1k_Plus;
                				  return new MinimalNfc_lpc_Commands(tag, prod);
                			  }
                		  }
                		  prod = Prod.NTAG_I2C_1k;
                		  return new MinimalNfc_lpc_Commands(tag, prod);
                	  } catch (Exception e2) {
                		  e2.printStackTrace();
                		  mfu.close();
                		  prod = Prod.NTAG_I2C_1k;
                		  return new MinimalNfc_lpc_Commands(tag, prod);
                	  }
                  } else if (answer[0] == (byte) 0x04 && answer[12] == (byte) 0xE1
                                && answer[13] == (byte) 0x10 && answer[14] == (byte) 0xEA
                                && answer[15] == (byte) 0x00) {
                	  // Try to read session registers to differentiate between standard and PLUS products
                	  try {	 
                		  answer = mfu.readPages(0xEC);
                		  mfu.close();
                		  for(int i = 0; i < 4; i++) {
                			  if(answer[i] != 0x00) {
                				  prod = Prod.NTAG_I2C_2k_Plus;
                				  return new MinimalNfc_lpc_Commands(tag, prod);
                			  }
                		  }
                		  prod = Prod.NTAG_I2C_2k;
                		  return new MinimalNfc_lpc_Commands(tag, prod);
                	  } catch (Exception e2) {
                		  e2.printStackTrace();
                		  mfu.close();
                		  prod = Prod.NTAG_I2C_2k;
                		  return new MinimalNfc_lpc_Commands(tag, prod);
                	  }
                  } else {
                	  mfu.close();
                	  return new MinimalNfc_lpc_Commands(tag, Prod.NTAG_I2C_1k_Plus);
                  }
            } catch (Exception e1) {
                  e1.printStackTrace();
                  mfu.close();
            }
		}
		return new MinimalNfc_lpc_Commands(tag, Prod.NTAG_I2C_1k_Plus);
	}

	protected int SRAMSize;


	protected int blockSize;

	public int getBlockSize() {
		return blockSize;
	}

	/**
	 * Closes the connection.
	 *
	 * @throws IOException
	 */
	public abstract void close() throws IOException;

	/**
	 * reopens the connection.
	 *
	 * @throws IOException
	 */
	public abstract void connect() throws IOException;

	/**
	 * reopens the connection.
	 *
	 * @throws IOException
	 */
	public abstract boolean isConnected();

	/**
	 * returns the last answer as Byte Array.
	 *
	 * @return Byte Array of the last Answer
	 */
	public abstract byte[] getLastAnswer();

	/**
	 * Gets the Product of the current Tag.
	 *
	 * @return Product of the Tag
	 * @throws IOException
	 */
	public abstract Prod getProduct() throws IOException;

	/**
	 * Writes Data to the EEPROM as long as enough space is on the Tag.
	 *
	 * @param data
	 *            Raw Data to write
	 * @throws IOException
	 * @throws FormatException
	 * @throws CommandNotSupportedException
	 */
	public abstract void writeEEPROM(byte[] data, WriteEEPROMListener listener) throws IOException,
			FormatException, CommandNotSupportedException;

	/**
	 * Writes Data to the EEPROM as long as enough space is on the Tag.
	 *
	 * @param data
	 *            Raw Data to write
	 * @throws IOException
	 * @throws FormatException
	 * @throws CommandNotSupportedException
	 */
	public abstract void writeEEPROMCustom(byte[] data, int startAddr,WriteEEPROMListener listener,boolean dummy) throws IOException,
			FormatException, CommandNotSupportedException;

	/**
	 * Writes Data to the EEPROM as long as enough space is on the Tag.
	 *
	 * @param data
	 *            Raw Data to write
	 * @param startAddr
	 *            Start Address from which the write begins
	 * @throws IOException
	 * @throws FormatException
	 */
	public abstract void writeEEPROM(int startAddr, byte[] data)
			throws IOException, FormatException;

	/**
	 * Read Data from the EEPROM.
	 *
	 * @param absStart
	 *            Start of the read
	 * @param absEnd
	 *            End of the read(included in the Answer)
	 * @return Data read
	 * @throws IOException
	 * @throws FormatException
	 * @throws CommandNotSupportedException
	 * 
	 */
	public abstract byte[] readEEPROM(int absStart, int absEnd)
			throws IOException, FormatException, CommandNotSupportedException;


	/**
	 * Write an Empty NDEF Message to the NTAG.
	 *
	 * @throws IOException
	 * @throws FormatException
	 */
	public abstract void writeEmptyNdef() throws IOException, FormatException;
	
	/**
	 * Write Default NDEF to the NTAG.
	 *
	 * @throws IOException
	 * @throws FormatException
	 */
	public abstract void writeDefaultNdef() throws IOException, FormatException;


	/**
	 * Read a NDEF Message from the tag - not an official NFC Forum NDEF
	 * detection routine.
	 *
	 * @param message
	 *            NDEF message to write on the tag
	 * @throws IOException
	 * @throws FormatException
	 * @throws CommandNotSupportedException
	 */
	public abstract void writeNDEF(NdefMessage message, WriteEEPROMListener listener) throws IOException,
			FormatException, CommandNotSupportedException;



	/**
	 * Read a NDEF Message from the tag - not an official NFC Forum NDEF
	 * detection routine.
	 *
	 * @throws IOException
	 * @throws FormatException
	 * @throws CommandNotSupportedException
	 */
	public abstract NdefMessage readNDEF() throws IOException, FormatException, CommandNotSupportedException;

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
}
