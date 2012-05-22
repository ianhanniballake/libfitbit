package com.ianhanniballake.fitbitsync.ui;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

import com.ianhanniballake.fitbit.Fitbit;
import com.ianhanniballake.fitbitsync.R;

public class FitbitSyncActivity extends Activity
{
	private UsbDevice device;
	private Fitbit fitbit = null;
	BroadcastReceiver usbReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(final Context context, final Intent intent)
		{
			final String action = intent.getAction();
			if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action))
			{
				final UsbDevice detachedDevice = intent
						.getParcelableExtra(UsbManager.EXTRA_DEVICE);
				final String deviceName = detachedDevice.getDeviceName();
				if (device != null && device.equals(deviceName))
				{
					fitbit.close();
					fitbit = null;
				}
			}
		}
	};

	@Override
	public void onCreate(final Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		final TextView log = (TextView) findViewById(R.id.log);
		final Button openConnection = (Button) findViewById(R.id.open_connection);
		openConnection.setOnClickListener(new OnClickListener()
		{
			@Override
			public void onClick(final View v)
			{
				final boolean success = fitbit.open();
				log.setText(log.getText() + "\n"
						+ getString(R.string.open_connection) + ": " + success);
			}
		});
		final Button initConnection = (Button) findViewById(R.id.init_connection);
		initConnection.setOnClickListener(new OnClickListener()
		{
			@Override
			public void onClick(final View v)
			{
				fitbit.init();
				log.setText(log.getText() + "\n"
						+ getString(R.string.init_connection) + ": " + true);
			}
		});
		final UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
		device = (UsbDevice) getIntent().getParcelableExtra(
				UsbManager.EXTRA_DEVICE);
		fitbit = new Fitbit(manager, device);
		log.setText("Found device " + device.getDeviceName());
		// listen for detach device intents
		final IntentFilter filter = new IntentFilter();
		filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
		registerReceiver(usbReceiver, filter);
	}
}