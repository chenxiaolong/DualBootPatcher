// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SELinuxSetLabelRequest extends Table {
  public static SELinuxSetLabelRequest getRootAsSELinuxSetLabelRequest(ByteBuffer _bb) { return getRootAsSELinuxSetLabelRequest(_bb, new SELinuxSetLabelRequest()); }
  public static SELinuxSetLabelRequest getRootAsSELinuxSetLabelRequest(ByteBuffer _bb, SELinuxSetLabelRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public SELinuxSetLabelRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String path() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer pathAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public String label() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer labelAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }
  public boolean followSymlinks() { int o = __offset(8); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }

  public static int createSELinuxSetLabelRequest(FlatBufferBuilder builder,
      int path,
      int label,
      boolean follow_symlinks) {
    builder.startObject(3);
    SELinuxSetLabelRequest.addLabel(builder, label);
    SELinuxSetLabelRequest.addPath(builder, path);
    SELinuxSetLabelRequest.addFollowSymlinks(builder, follow_symlinks);
    return SELinuxSetLabelRequest.endSELinuxSetLabelRequest(builder);
  }

  public static void startSELinuxSetLabelRequest(FlatBufferBuilder builder) { builder.startObject(3); }
  public static void addPath(FlatBufferBuilder builder, int pathOffset) { builder.addOffset(0, pathOffset, 0); }
  public static void addLabel(FlatBufferBuilder builder, int labelOffset) { builder.addOffset(1, labelOffset, 0); }
  public static void addFollowSymlinks(FlatBufferBuilder builder, boolean followSymlinks) { builder.addBoolean(2, followSymlinks, false); }
  public static int endSELinuxSetLabelRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

