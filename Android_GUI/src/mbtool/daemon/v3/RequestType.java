// automatically generated, do not modify

package mbtool.daemon.v3;

public final class RequestType {
  private RequestType() { }
  public static final byte NONE = 0;
  public static final byte FileChmodRequest = 1;
  public static final byte FileCloseRequest = 2;
  public static final byte FileOpenRequest = 3;
  public static final byte FileReadRequest = 4;
  public static final byte FileSeekRequest = 5;
  public static final byte FileStatRequest = 6;
  public static final byte FileWriteRequest = 7;
  public static final byte FileSELinuxGetLabelRequest = 8;
  public static final byte FileSELinuxSetLabelRequest = 9;
  public static final byte PathChmodRequest = 10;
  public static final byte PathCopyRequest = 11;
  public static final byte PathSELinuxGetLabelRequest = 12;
  public static final byte PathSELinuxSetLabelRequest = 13;
  public static final byte PathGetDirectorySizeRequest = 14;
  public static final byte MbGetVersionRequest = 15;
  public static final byte MbGetInstalledRomsRequest = 16;
  public static final byte MbGetBootedRomIdRequest = 17;
  public static final byte MbSwitchRomRequest = 18;
  public static final byte MbSetKernelRequest = 19;
  public static final byte MbWipeRomRequest = 20;
  public static final byte MbGetPackagesCountRequest = 21;
  public static final byte RebootRequest = 22;
  public static final byte SignedExecRequest = 23;
  public static final byte ShutdownRequest = 24;
  public static final byte PathDeleteRequest = 25;
  public static final byte PathMkdirRequest = 26;
  public static final byte CryptoDecryptRequest = 27;
  public static final byte CryptoGetPwTypeRequest = 28;

  private static final String[] names = { "NONE", "FileChmodRequest", "FileCloseRequest", "FileOpenRequest", "FileReadRequest", "FileSeekRequest", "FileStatRequest", "FileWriteRequest", "FileSELinuxGetLabelRequest", "FileSELinuxSetLabelRequest", "PathChmodRequest", "PathCopyRequest", "PathSELinuxGetLabelRequest", "PathSELinuxSetLabelRequest", "PathGetDirectorySizeRequest", "MbGetVersionRequest", "MbGetInstalledRomsRequest", "MbGetBootedRomIdRequest", "MbSwitchRomRequest", "MbSetKernelRequest", "MbWipeRomRequest", "MbGetPackagesCountRequest", "RebootRequest", "SignedExecRequest", "ShutdownRequest", "PathDeleteRequest", "PathMkdirRequest", "CryptoDecryptRequest", "CryptoGetPwTypeRequest", };

  public static String name(int e) { return names[e]; }
};

