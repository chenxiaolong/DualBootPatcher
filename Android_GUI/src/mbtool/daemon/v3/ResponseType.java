// automatically generated, do not modify

package mbtool.daemon.v3;

public final class ResponseType {
  private ResponseType() { }
  public static final byte NONE = 0;
  public static final byte Invalid = 1;
  public static final byte Unsupported = 2;
  public static final byte FileChmodResponse = 3;
  public static final byte FileCloseResponse = 4;
  public static final byte FileOpenResponse = 5;
  public static final byte FileReadResponse = 6;
  public static final byte FileSeekResponse = 7;
  public static final byte FileStatResponse = 8;
  public static final byte FileWriteResponse = 9;
  public static final byte FileSELinuxGetLabelResponse = 10;
  public static final byte FileSELinuxSetLabelResponse = 11;
  public static final byte PathChmodResponse = 12;
  public static final byte PathCopyResponse = 13;
  public static final byte PathSELinuxGetLabelResponse = 14;
  public static final byte PathSELinuxSetLabelResponse = 15;
  public static final byte PathGetDirectorySizeResponse = 16;
  public static final byte MbGetVersionResponse = 17;
  public static final byte MbGetInstalledRomsResponse = 18;
  public static final byte MbGetBootedRomIdResponse = 19;
  public static final byte MbSwitchRomResponse = 20;
  public static final byte MbSetKernelResponse = 21;
  public static final byte MbWipeRomResponse = 22;
  public static final byte MbGetPackagesCountResponse = 23;
  public static final byte RebootResponse = 24;
  public static final byte SignedExecOutputResponse = 25;
  public static final byte SignedExecResponse = 26;
  public static final byte ShutdownResponse = 27;
  public static final byte PathDeleteResponse = 28;
  public static final byte PathMkdirResponse = 29;
  public static final byte CryptoDecryptResponse = 30;
  public static final byte CryptoGetPwTypeResponse = 31;

  private static final String[] names = { "NONE", "Invalid", "Unsupported", "FileChmodResponse", "FileCloseResponse", "FileOpenResponse", "FileReadResponse", "FileSeekResponse", "FileStatResponse", "FileWriteResponse", "FileSELinuxGetLabelResponse", "FileSELinuxSetLabelResponse", "PathChmodResponse", "PathCopyResponse", "PathSELinuxGetLabelResponse", "PathSELinuxSetLabelResponse", "PathGetDirectorySizeResponse", "MbGetVersionResponse", "MbGetInstalledRomsResponse", "MbGetBootedRomIdResponse", "MbSwitchRomResponse", "MbSetKernelResponse", "MbWipeRomResponse", "MbGetPackagesCountResponse", "RebootResponse", "SignedExecOutputResponse", "SignedExecResponse", "ShutdownResponse", "PathDeleteResponse", "PathMkdirResponse", "CryptoDecryptResponse", "CryptoGetPwTypeResponse", };

  public static String name(int e) { return names[e]; }
};

