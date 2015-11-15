// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class FileReadResponse extends Table {
  public static FileReadResponse getRootAsFileReadResponse(ByteBuffer _bb) { return getRootAsFileReadResponse(_bb, new FileReadResponse()); }
  public static FileReadResponse getRootAsFileReadResponse(ByteBuffer _bb, FileReadResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public FileReadResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }
  public String errorMsg() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer errorMsgAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }
  public long bytesRead() { int o = __offset(8); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public int data(int j) { int o = __offset(10); return o != 0 ? bb.get(__vector(o) + j * 1) & 0xFF : 0; }
  public int dataLength() { int o = __offset(10); return o != 0 ? __vector_len(o) : 0; }
  public ByteBuffer dataAsByteBuffer() { return __vector_as_bytebuffer(10, 1); }

  public static int createFileReadResponse(FlatBufferBuilder builder,
      boolean success,
      int error_msg,
      long bytes_read,
      int data) {
    builder.startObject(4);
    FileReadResponse.addBytesRead(builder, bytes_read);
    FileReadResponse.addData(builder, data);
    FileReadResponse.addErrorMsg(builder, error_msg);
    FileReadResponse.addSuccess(builder, success);
    return FileReadResponse.endFileReadResponse(builder);
  }

  public static void startFileReadResponse(FlatBufferBuilder builder) { builder.startObject(4); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static void addErrorMsg(FlatBufferBuilder builder, int errorMsgOffset) { builder.addOffset(1, errorMsgOffset, 0); }
  public static void addBytesRead(FlatBufferBuilder builder, long bytesRead) { builder.addLong(2, bytesRead, 0); }
  public static void addData(FlatBufferBuilder builder, int dataOffset) { builder.addOffset(3, dataOffset, 0); }
  public static int createDataVector(FlatBufferBuilder builder, byte[] data) { builder.startVector(1, data.length, 1); for (int i = data.length - 1; i >= 0; i--) builder.addByte(data[i]); return builder.endVector(); }
  public static void startDataVector(FlatBufferBuilder builder, int numElems) { builder.startVector(1, numElems, 1); }
  public static int endFileReadResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

