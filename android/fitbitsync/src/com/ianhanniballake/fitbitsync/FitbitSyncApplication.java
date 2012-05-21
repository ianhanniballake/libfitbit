package com.ianhanniballake.fitbitsync;

import org.acra.ACRA;
import org.acra.annotation.ReportsCrashes;

import android.app.Application;
import android.os.StrictMode;

/**
 * Creates the Fitbit Sync application, setting strict mode in debug mode
 */
@ReportsCrashes(formKey = "dEpESUMycDJ5bHBDSmZ4OWhIeENHNmc6MA")
public class FitbitSyncApplication extends Application
{
	/**
	 * Sets strict mode if we are in debug mode, init ACRA if we are not.
	 * 
	 * @see android.app.Application#onCreate()
	 */
	@Override
	public void onCreate()
	{
		if (BuildConfig.DEBUG)
		{
			final StrictMode.ThreadPolicy.Builder threadPolicy = new StrictMode.ThreadPolicy.Builder()
					.detectAll().penaltyLog();
			StrictMode.setThreadPolicy(threadPolicy.build());
			StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
					.detectAll().penaltyLog().build());
		}
		else
			ACRA.init(this);
		super.onCreate();
	}
}