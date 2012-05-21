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
	private final UsbDevice device;
	private final UsbManager manager;

	public Fitbit(final UsbManager manager, final UsbDevice device)
	{
		this.manager = manager;
		this.device = device;
	}

	public void close()
	{
		if (ant != null)
			ant.closeConnection();
		ant = null;
	}

	public void init()
	{
		ant.controlTransfer(0x40, 0x00, 0xFFFF, 0x00, new byte[0]);
		ant.controlTransfer(0x40, 0x01, 0x2000, 0x00, new byte[0]);
		ant.controlTransfer(0x40, 0x00, 0x00, 0x00, new byte[0]);
		ant.controlTransfer(0x40, 0x00, 0xFFFF, 0x00, new byte[0]);
		ant.controlTransfer(0x40, 0x01, 0x2000, 0x00, new byte[0]);
		ant.controlTransfer(0x40, 0x01, 0x4A, 0x00, new byte[0]);
		ant.controlTransfer(0xC0, 0xFF, 0x370B, 0x00, 1);
		ant.controlTransfer(0x40, 0x03, 0x800, 0x00, new byte[0]);
		ant.controlTransfer(0x40, 0x13, 0x00, 0x00, new byte[] { 0x08, 0x00,
				0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00 });
		ant.controlTransfer(0x40, 0x12, 0x0C, 0x00, new byte[0]);
		ant.read(new byte[4096], 4096);
	}

	public boolean open()
	{
		final UsbInterface intf = device.getInterface(Fitbit.INTERFACE_INDEX);
		final UsbDeviceConnection connection = manager.openDevice(device);
		final boolean claimed = connection.claimInterface(intf, true);
		if (claimed)
			ant = new ANT(connection, intf);
		return claimed;
	}
}
