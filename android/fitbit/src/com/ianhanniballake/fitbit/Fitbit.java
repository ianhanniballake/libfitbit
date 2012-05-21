package com.ianhanniballake.fitbit;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;

import com.ianhaniballake.antprotocol.ANT;

public class Fitbit
{
	private static final int INTERFACE_INDEX = 0;
	private ANT ant = null;
	private UsbDeviceConnection connection;
	private final UsbDevice device;
	private UsbInterface intf;
	private final UsbManager manager;

	public Fitbit(final UsbManager manager, final UsbDevice device)
	{
		this.manager = manager;
		this.device = device;
	}

	public void close()
	{
		connection.releaseInterface(intf);
		if (ant != null)
			ant.closeConnection();
		ant = null;
	}

	public boolean open()
	{
		intf = device.getInterface(Fitbit.INTERFACE_INDEX);
		connection = manager.openDevice(device);
		final boolean claimed = connection.claimInterface(intf, true);
		if (claimed)
			ant = new ANT(connection);
		return claimed;
	}
}
