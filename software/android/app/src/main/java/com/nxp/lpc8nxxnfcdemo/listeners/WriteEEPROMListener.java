package com.nxp.lpc8nxxnfcdemo.listeners;

public interface WriteEEPROMListener {
	/**
	 * It informs the listener about the number of bytes written in the EEPROM
	 * Used to inform about the progress during the SpeedTest
	 * 
	 * @param bytes
	 */
    public abstract void onWriteEEPROM(int bytes);
}
