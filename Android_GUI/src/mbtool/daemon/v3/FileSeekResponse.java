// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class FileSeekResponse extends Table {
  public static FileSeekResponse getRootAsFileSeekResponse(ByteBuffer _bb) { return getRootAsFileSeekResponse(_bb, new FileSeekResponse()); }
  public static FileSeekResponse getRootAsFileSeekResponse(ByteBuffer _bb, FileSeekResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public FileSeekResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }
  public String errorMsg() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer errorMsgAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }
  public long offset() { int o = __offset(8); return o != 0 ? bb.getLong(o + bb_pos) : 0; }

  public static int createFileSeekResponse(FlatBufferBuilder builder,
      boolean success,
      int error_msg,
      long offset) {
    builder.startObject(3);
    FileSeekResponse.addOffset(builder, offset);
    FileSeekResponse.addErrorMsg(builder, error_msg);
    FileSeekResponse.addSuccess(builder, success);
    return FileSeekResponse.endFileSeekResponse(builder);
  }

  public static void startFileSeekResponse(FlatBufferBuilder builder) { builder.startObject(3); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static void addErrorMsg(FlatBufferBuilder builder, int errorMsgOffset) { builder.addOffset(1, errorMsgOffset, 0); }
  public static void addOffset(FlatBufferBuilder builder, long offset) { builder.addLong(2, offset, 0); }
  public static int endFileSeekResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

