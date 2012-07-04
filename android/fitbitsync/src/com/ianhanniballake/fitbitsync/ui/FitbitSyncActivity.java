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

import com.google.analytics.tracking.android.EasyTracker;
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
			if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action))
			{
				final UsbDevice attachedDevice = intent
						.getParcelableExtra(UsbManager.EXTRA_DEVICE);
				deviceConnected(attachedDevice);
			}
			if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action))
			{
				final UsbDevice detachedDevice = intent
						.getParcelableExtra(UsbManager.EXTRA_DEVICE);
				final String deviceName = detachedDevice.getDeviceName();
				if (device != null && device.equals(deviceName))
				{
					fitbit.close();
					EasyTracker.getTracker().trackEvent("device", "disconnect",
							"", 0L);
					fitbit = null;
					final Button openConnection = (Button) findViewById(R.id.open_connection);
					openConnection.setEnabled(false);
					final Button initConnection = (Button) findViewById(R.id.init_connection);
					initConnection.setEnabled(false);
				}
			}
		}
	};

	private void deviceConnected(final UsbDevice attachedDevice)
	{
		final UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
		device = attachedDevice;
		fitbit = new Fitbit(manager, device);
		final TextView log = (TextView) findViewById(R.id.log);
		log.setText(log.getText() + "\nFound device " + device.getDeviceName());
		final Button openConnection = (Button) findViewById(R.id.open_connection);
		openConnection.setEnabled(true);
		final Button initConnection = (Button) findViewById(R.id.init_connection);
		initConnection.setEnabled(true);
	}

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
				EasyTracker.getTracker().trackEvent("device", "open",
						Boolean.toString(success), 0L);
				log.setText(log.getText() + "\n"
						+ getString(R.string.open_connection) + ": " + success);
			}
		});
		openConnection.setEnabled(false);
		final Button initConnection = (Button) findViewById(R.id.init_connection);
		initConnection.setOnClickListener(new OnClickListener()
		{
			@Override
			public void onClick(final View v)
			{
				fitbit.init();
				EasyTracker.getTracker().trackEvent("device", "init", "", 0L);
				log.setText(log.getText() + "\n"
						+ getString(R.string.init_connection) + ": " + true);
			}
		});
		initConnection.setEnabled(false);
		// listen for device intents
		final IntentFilter filter = new IntentFilter();
		filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
		filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
		registerReceiver(usbReceiver, filter);
		device = (UsbDevice) getIntent().getParcelableExtra(
				UsbManager.EXTRA_DEVICE);
		if (device != null)
			deviceConnected(device);
	}

	@Override
	protected void onStart()
	{
		super.onStart();
		EasyTracker.getInstance().activityStart(this);
	}

	@Override
	protected void onStop()
	{
		super.onStop();
		EasyTracker.getInstance().activityStop(this);
	}
}