// automatically generated, do not modify

package mbtool.daemon.v2;

public class ResponseType {
  public static final short UNSUPPORTED = 0;
  public static final short INVALID = 1;
  public static final short GET_VERSION = 2;
  public static final short GET_ROMS_LIST = 3;
  public static final short GET_BUILTIN_ROM_IDS = 4;
  public static final short GET_CURRENT_ROM = 5;
  public static final short SWITCH_ROM = 6;
  public static final short SET_KERNEL = 7;
  public static final short REBOOT = 8;
  public static final short OPEN = 9;
  public static final short COPY = 10;
  public static final short CHMOD = 11;
  public static final short LOKI_PATCH = 12;
  public static final short WIPE_ROM = 13;

  private static final String[] names = { "UNSUPPORTED", "INVALID", "GET_VERSION", "GET_ROMS_LIST", "GET_BUILTIN_ROM_IDS", "GET_CURRENT_ROM", "SWITCH_ROM", "SET_KERNEL", "REBOOT", "OPEN", "COPY", "CHMOD", "LOKI_PATCH", "WIPE_ROM", };

  public static String name(int e) { return names[e]; }
};

