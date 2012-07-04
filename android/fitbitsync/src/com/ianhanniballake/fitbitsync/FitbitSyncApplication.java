package com.ianhanniballake.fitbitsync;

import java.lang.Thread.UncaughtExceptionHandler;

import org.acra.ACRA;
import org.acra.annotation.ReportsCrashes;

import android.app.Application;
import android.os.StrictMode;

import com.google.analytics.tracking.android.EasyTracker;
import com.google.analytics.tracking.android.ExceptionReporter;
import com.google.analytics.tracking.android.GAServiceManager;
import com.google.analytics.tracking.android.GoogleAnalytics;

/**
 * Creates the Fitbit Sync application, setting strict mode in debug mode
 */
@ReportsCrashes(formKey = "dEpESUMycDJ5bHBDSmZ4OWhIeENHNmc6MA")
public class FitbitSyncApplication extends Application
{
	/**
	 * Sets strict mode if we are in debug mode, init Google Analytics / ACRA if
	 * we are not.
	 * 
	 * @see android.app.Application#onCreate()
	 */
	@Override
	public void onCreate()
	{
		GoogleAnalytics.getInstance(this).setDebug(BuildConfig.DEBUG);
		if (BuildConfig.DEBUG)
		{
			final StrictMode.ThreadPolicy.Builder threadPolicy = new StrictMode.ThreadPolicy.Builder()
					.detectAll().penaltyLog();
			StrictMode.setThreadPolicy(threadPolicy.build());
			StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
					.detectAll().penaltyLog().build());
		}
		else
		{
			// Initialize Google Analytics Error Handling first
			final UncaughtExceptionHandler myHandler = new ExceptionReporter(
					EasyTracker.getTracker(), GAServiceManager.getInstance(),
					Thread.getDefaultUncaughtExceptionHandler());
			Thread.setDefaultUncaughtExceptionHandler(myHandler);
			ACRA.init(this);
		}
		super.onCreate();
	}
}