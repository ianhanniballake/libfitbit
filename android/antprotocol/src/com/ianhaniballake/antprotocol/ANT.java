package com.ianhaniballake.antprotocol;

import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;

public class ANT
{
	private static final int READ_TIMEOUT = 1000;
	private static final int WRITE_TIMEOUT = 100;
	private final UsbDeviceConnection connection;
	private final UsbInterface intf;
	private final UsbEndpoint mEndpointIn;
	private final UsbEndpoint mEndpointOut;

	public ANT(final UsbDeviceConnection connection, final UsbInterface intf)
	{
		this.connection = connection;
		this.intf = intf;
		UsbEndpoint epOut = null;
		UsbEndpoint epIn = null;
		// look for our bulk endpoints
		for (int i = 0; i < intf.getEndpointCount(); i++)
		{
			final UsbEndpoint ep = intf.getEndpoint(i);
			if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK)
				if (ep.getDirection() == UsbConstants.USB_DIR_OUT)
					epOut = ep;
				else
					epIn = ep;
		}
		if (epOut == null || epIn == null)
			throw new IllegalArgumentException("not all endpoints found");
		mEndpointOut = epOut;
		mEndpointIn = epIn;
	}

	public void closeConnection()
	{
		connection.releaseInterface(intf);
		connection.close();
	}

	public void controlTransfer(final int requestType, final int request,
			final int value, final int index, final byte[] buffer)
	{
		connection.controlTransfer(requestType, request, value, index, buffer,
				buffer.length, ANT.WRITE_TIMEOUT);
	}

	public void controlTransfer(final int requestType, final int request,
			final int value, final int index, final int length)
	{
		connection.controlTransfer(requestType, request, value, index, null,
				length, ANT.WRITE_TIMEOUT);
	}

	public int read(final byte[] data, final int length)
	{
		return connection.bulkTransfer(mEndpointIn, data, length,
				ANT.READ_TIMEOUT);
	}

	public int write(final byte[] buffer)
	{
		return connection.bulkTransfer(mEndpointOut, buffer, buffer.length,
				ANT.WRITE_TIMEOUT);
	}
}
