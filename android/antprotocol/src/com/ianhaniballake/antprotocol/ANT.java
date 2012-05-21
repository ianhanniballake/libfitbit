package com.ianhaniballake.antprotocol;

import android.hardware.usb.UsbDeviceConnection;

public class ANT
{
	private final UsbDeviceConnection connection;

	public ANT(final UsbDeviceConnection connection)
	{
		this.connection = connection;
	}

	public void closeConnection()
	{
		connection.close();
	}
}
