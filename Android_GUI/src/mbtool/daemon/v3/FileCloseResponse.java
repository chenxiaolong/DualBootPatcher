// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class FileCloseResponse extends Table {
  public static FileCloseResponse getRootAsFileCloseResponse(ByteBuffer _bb) { return getRootAsFileCloseResponse(_bb, new FileCloseResponse()); }
  public static FileCloseResponse getRootAsFileCloseResponse(ByteBuffer _bb, FileCloseResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public FileCloseResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }
  public String errorMsg() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer errorMsgAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }

  public static int createFileCloseResponse(FlatBufferBuilder builder,
      boolean success,
      int error_msgOffset) {
    builder.startObject(2);
    FileCloseResponse.addErrorMsg(builder, error_msgOffset);
    FileCloseResponse.addSuccess(builder, success);
    return FileCloseResponse.endFileCloseResponse(builder);
  }

  public static void startFileCloseResponse(FlatBufferBuilder builder) { builder.startObject(2); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static void addErrorMsg(FlatBufferBuilder builder, int errorMsgOffset) { builder.addOffset(1, errorMsgOffset, 0); }
  public static int endFileCloseResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

