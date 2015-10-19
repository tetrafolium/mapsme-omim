package com.mapswithme.maps.search;

import com.mapswithme.util.concurrency.UiThread;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

public enum SearchEngine implements NativeSearchListener
{
  INSTANCE;

  @Override
  public void onResultsUpdate(final SearchResult[] results, final long timestamp)
  {
    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        for (NativeSearchListener listener : mListeners)
          listener.onResultsUpdate(results, timestamp);
      }
    });
  }

  @Override
  public void onResultsEnd(final long timestamp)
  {
    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        for (NativeSearchListener listener : mListeners)
          listener.onResultsEnd(timestamp);
      }
    });
  }

  private List<NativeSearchListener> mListeners = new ArrayList<>();

  public void addListener(NativeSearchListener listener)
  {
    mListeners.add(listener);
  }

  public boolean removeListener(NativeSearchListener listener)
  {
    return mListeners.remove(listener);
  }

  SearchEngine()
  {
    nativeInit();
  }

  private native void nativeInit();

  /**
   * @param timestamp Search results are filtered according to it after multiple requests.
   * @param force     Should be false for repeating requests with the same query.
   * @return whether search was actually started.
   */
  public static boolean runSearch(String query, String language, long timestamp, boolean force, boolean hasLocation, double lat, double lon)
  {
    try
    {
      return nativeRunSearch(query.getBytes("utf-8"), language, timestamp, force, hasLocation, lat, lon);
    } catch (UnsupportedEncodingException ignored) { }

    return false;
  }

  public static void runInteractiveSearch(String query, String language, long timestamp)
  {
    try
    {
      nativeRunInteractiveSearch(query.getBytes("utf-8"), language, timestamp);
    } catch (UnsupportedEncodingException ignored) { }
  }

  /**
   * @param bytes utf-8 formatted bytes of query.
   */
  private static native boolean nativeRunSearch(byte[] bytes, String language, long timestamp, boolean force, boolean hasLocation, double lat, double lon);

  /**
   * @param bytes utf-8 formatted query bytes
   */
  private static native void nativeRunInteractiveSearch(byte[] bytes, String language, long timestamp);

  public static native void nativeShowResult(int index);

  public static native void nativeShowAllResults();
}