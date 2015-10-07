// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SELinuxGetLabelRequest extends Table {
  public static SELinuxGetLabelRequest getRootAsSELinuxGetLabelRequest(ByteBuffer _bb) { return getRootAsSELinuxGetLabelRequest(_bb, new SELinuxGetLabelRequest()); }
  public static SELinuxGetLabelRequest getRootAsSELinuxGetLabelRequest(ByteBuffer _bb, SELinuxGetLabelRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public SELinuxGetLabelRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String path() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer pathAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public boolean followSymlinks() { int o = __offset(6); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }

  public static int createSELinuxGetLabelRequest(FlatBufferBuilder builder,
      int path,
      boolean follow_symlinks) {
    builder.startObject(2);
    SELinuxGetLabelRequest.addPath(builder, path);
    SELinuxGetLabelRequest.addFollowSymlinks(builder, follow_symlinks);
    return SELinuxGetLabelRequest.endSELinuxGetLabelRequest(builder);
  }

  public static void startSELinuxGetLabelRequest(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addPath(FlatBufferBuilder builder, int pathOffset) { builder.addOffset(0, pathOffset, 0); }
  public static void addFollowSymlinks(FlatBufferBuilder builder, boolean followSymlinks) { builder.addBoolean(1, followSymlinks, false); }
  public static int endSELinuxGetLabelRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

