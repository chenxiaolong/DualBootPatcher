// automatically generated, do not modify

package mbtool.daemon.v2;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

public class Rom extends Table {
  public static Rom getRootAsRom(ByteBuffer _bb) { return getRootAsRom(_bb, new Rom()); }
  public static Rom getRootAsRom(ByteBuffer _bb, Rom obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public Rom __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String id() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer idAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public String systemPath() { int o = __offset(6); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer systemPathAsByteBuffer() { return __vector_as_bytebuffer(6, 1); }
  public String cachePath() { int o = __offset(8); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer cachePathAsByteBuffer() { return __vector_as_bytebuffer(8, 1); }
  public String dataPath() { int o = __offset(10); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer dataPathAsByteBuffer() { return __vector_as_bytebuffer(10, 1); }
  public String version() { int o = __offset(12); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer versionAsByteBuffer() { return __vector_as_bytebuffer(12, 1); }
  public String build() { int o = __offset(14); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer buildAsByteBuffer() { return __vector_as_bytebuffer(14, 1); }

  public static int createRom(FlatBufferBuilder builder,
      int id,
      int system_path,
      int cache_path,
      int data_path,
      int version,
      int build) {
    builder.startObject(6);
    Rom.addBuild(builder, build);
    Rom.addVersion(builder, version);
    Rom.addDataPath(builder, data_path);
    Rom.addCachePath(builder, cache_path);
    Rom.addSystemPath(builder, system_path);
    Rom.addId(builder, id);
    return Rom.endRom(builder);
  }

  public static void startRom(FlatBufferBuilder builder) { builder.startObject(6); }
  public static void addId(FlatBufferBuilder builder, int idOffset) { builder.addOffset(0, idOffset, 0); }
  public static void addSystemPath(FlatBufferBuilder builder, int systemPathOffset) { builder.addOffset(1, systemPathOffset, 0); }
  public static void addCachePath(FlatBufferBuilder builder, int cachePathOffset) { builder.addOffset(2, cachePathOffset, 0); }
  public static void addDataPath(FlatBufferBuilder builder, int dataPathOffset) { builder.addOffset(3, dataPathOffset, 0); }
  public static void addVersion(FlatBufferBuilder builder, int versionOffset) { builder.addOffset(4, versionOffset, 0); }
  public static void addBuild(FlatBufferBuilder builder, int buildOffset) { builder.addOffset(5, buildOffset, 0); }
  public static int endRom(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

