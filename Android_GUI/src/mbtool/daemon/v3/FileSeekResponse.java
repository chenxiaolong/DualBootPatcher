// automatically generated by the FlatBuffers compiler, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class FileSeekResponse extends Table {
  public static FileSeekResponse getRootAsFileSeekResponse(ByteBuffer _bb) { return getRootAsFileSeekResponse(_bb, new FileSeekResponse()); }
  public static FileSeekResponse getRootAsFileSeekResponse(ByteBuffer _bb, FileSeekResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__assign(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public void __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; }
  public FileSeekResponse __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }
  public String errorMsg() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer errorMsgAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }
  public ByteBuffer errorMsgInByteBuffer(ByteBuffer _bb) { return __vector_in_bytebuffer(_bb, 6, 1); }
  public long offset() { int o = __offset(8); return o != 0 ? bb.getLong(o + bb_pos) : 0L; }
  public FileSeekError error() { return error(new FileSeekError()); }
  public FileSeekError error(FileSeekError obj) { int o = __offset(10); return o != 0 ? obj.__assign(__indirect(o + bb_pos), bb) : null; }

  public static int createFileSeekResponse(FlatBufferBuilder builder,
      boolean success,
      int error_msgOffset,
      long offset,
      int errorOffset) {
    builder.startObject(4);
    FileSeekResponse.addOffset(builder, offset);
    FileSeekResponse.addError(builder, errorOffset);
    FileSeekResponse.addErrorMsg(builder, error_msgOffset);
    FileSeekResponse.addSuccess(builder, success);
    return FileSeekResponse.endFileSeekResponse(builder);
  }

  public static void startFileSeekResponse(FlatBufferBuilder builder) { builder.startObject(4); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static void addErrorMsg(FlatBufferBuilder builder, int errorMsgOffset) { builder.addOffset(1, errorMsgOffset, 0); }
  public static void addOffset(FlatBufferBuilder builder, long offset) { builder.addLong(2, offset, 0L); }
  public static void addError(FlatBufferBuilder builder, int errorOffset) { builder.addOffset(3, errorOffset, 0); }
  public static int endFileSeekResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
}

