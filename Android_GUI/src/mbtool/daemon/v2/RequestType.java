// automatically generated, do not modify

package mbtool.daemon.v2;

public class RequestType {
  public static final short GET_VERSION = 0;
  public static final short GET_ROMS_LIST = 1;
  public static final short GET_BUILTIN_ROM_IDS = 2;
  public static final short GET_CURRENT_ROM = 3;
  public static final short SWITCH_ROM = 4;
  public static final short SET_KERNEL = 5;
  public static final short REBOOT = 6;
  public static final short OPEN = 7;
  public static final short COPY = 8;
  public static final short CHMOD = 9;
  public static final short LOKI_PATCH = 10;
  public static final short WIPE_ROM = 11;

  private static final String[] names = { "GET_VERSION", "GET_ROMS_LIST", "GET_BUILTIN_ROM_IDS", "GET_CURRENT_ROM", "SWITCH_ROM", "SET_KERNEL", "REBOOT", "OPEN", "COPY", "CHMOD", "LOKI_PATCH", "WIPE_ROM", };

  public static String name(int e) { return names[e]; }
};

