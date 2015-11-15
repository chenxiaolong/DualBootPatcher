// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class FileSeekRequest extends Table {
  public static FileSeekRequest getRootAsFileSeekRequest(ByteBuffer _bb) { return getRootAsFileSeekRequest(_bb, new FileSeekRequest()); }
  public static FileSeekRequest getRootAsFileSeekRequest(ByteBuffer _bb, FileSeekRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public FileSeekRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public int id() { int o = __offset(4); return o != 0 ? bb.getInt(o + bb_pos) : 0; }
  public long offset() { int o = __offset(6); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public short whence() { int o = __offset(8); return o != 0 ? bb.getShort(o + bb_pos) : 0; }

  public static int createFileSeekRequest(FlatBufferBuilder builder,
      int id,
      long offset,
      short whence) {
    builder.startObject(3);
    FileSeekRequest.addOffset(builder, offset);
    FileSeekRequest.addId(builder, id);
    FileSeekRequest.addWhence(builder, whence);
    return FileSeekRequest.endFileSeekRequest(builder);
  }

  public static void startFileSeekRequest(FlatBufferBuilder builder) { builder.startObject(3); }
  public static void addId(FlatBufferBuilder builder, int id) { builder.addInt(0, id, 0); }
  public static void addOffset(FlatBufferBuilder builder, long offset) { builder.addLong(1, offset, 0); }
  public static void addWhence(FlatBufferBuilder builder, short whence) { builder.addShort(2, whence, 0); }
  public static int endFileSeekRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

