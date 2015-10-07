// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class SELinuxGetLabelResponse extends Table {
  public static SELinuxGetLabelResponse getRootAsSELinuxGetLabelResponse(ByteBuffer _bb) { return getRootAsSELinuxGetLabelResponse(_bb, new SELinuxGetLabelResponse()); }
  public static SELinuxGetLabelResponse getRootAsSELinuxGetLabelResponse(ByteBuffer _bb, SELinuxGetLabelResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public SELinuxGetLabelResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }
  public String label() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer labelAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }
  public String errorMsg() { int o = __offset(8); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer errorMsgAsByteBuffer() { return __vector_as_bytebuffer(8, 1); }

  public static int createSELinuxGetLabelResponse(FlatBufferBuilder builder,
      boolean success,
      int label,
      int error_msg) {
    builder.startObject(3);
    SELinuxGetLabelResponse.addErrorMsg(builder, error_msg);
    SELinuxGetLabelResponse.addLabel(builder, label);
    SELinuxGetLabelResponse.addSuccess(builder, success);
    return SELinuxGetLabelResponse.endSELinuxGetLabelResponse(builder);
  }

  public static void startSELinuxGetLabelResponse(FlatBufferBuilder builder) { builder.startObject(3); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static void addLabel(FlatBufferBuilder builder, int labelOffset) { builder.addOffset(1, labelOffset, 0); }
  public static void addErrorMsg(FlatBufferBuilder builder, int errorMsgOffset) { builder.addOffset(2, errorMsgOffset, 0); }
  public static int endSELinuxGetLabelResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

