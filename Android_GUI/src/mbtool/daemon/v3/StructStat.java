// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class StructStat extends Table {
  public static StructStat getRootAsStructStat(ByteBuffer _bb) { return getRootAsStructStat(_bb, new StructStat()); }
  public static StructStat getRootAsStructStat(ByteBuffer _bb, StructStat obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public StructStat __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public long stDev() { int o = __offset(4); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public long stIno() { int o = __offset(6); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public long stMode() { int o = __offset(8); return o != 0 ? (long)bb.getInt(o + bb_pos) & 0xFFFFFFFFL : 0; }
  public long stNlink() { int o = __offset(10); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public long stUid() { int o = __offset(12); return o != 0 ? (long)bb.getInt(o + bb_pos) & 0xFFFFFFFFL : 0; }
  public long stGid() { int o = __offset(14); return o != 0 ? (long)bb.getInt(o + bb_pos) & 0xFFFFFFFFL : 0; }
  public long stRdev() { int o = __offset(16); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public long stSize() { int o = __offset(18); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public long stBlksize() { int o = __offset(20); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public long stBlocks() { int o = __offset(22); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public long stAtime() { int o = __offset(24); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public long stMtime() { int o = __offset(26); return o != 0 ? bb.getLong(o + bb_pos) : 0; }
  public long stCtime() { int o = __offset(28); return o != 0 ? bb.getLong(o + bb_pos) : 0; }

  public static int createStructStat(FlatBufferBuilder builder,
      long st_dev,
      long st_ino,
      long st_mode,
      long st_nlink,
      long st_uid,
      long st_gid,
      long st_rdev,
      long st_size,
      long st_blksize,
      long st_blocks,
      long st_atime,
      long st_mtime,
      long st_ctime) {
    builder.startObject(13);
    StructStat.addStCtime(builder, st_ctime);
    StructStat.addStMtime(builder, st_mtime);
    StructStat.addStAtime(builder, st_atime);
    StructStat.addStBlocks(builder, st_blocks);
    StructStat.addStBlksize(builder, st_blksize);
    StructStat.addStSize(builder, st_size);
    StructStat.addStRdev(builder, st_rdev);
    StructStat.addStNlink(builder, st_nlink);
    StructStat.addStIno(builder, st_ino);
    StructStat.addStDev(builder, st_dev);
    StructStat.addStGid(builder, st_gid);
    StructStat.addStUid(builder, st_uid);
    StructStat.addStMode(builder, st_mode);
    return StructStat.endStructStat(builder);
  }

  public static void startStructStat(FlatBufferBuilder builder) { builder.startObject(13); }
  public static void addStDev(FlatBufferBuilder builder, long stDev) { builder.addLong(0, stDev, 0); }
  public static void addStIno(FlatBufferBuilder builder, long stIno) { builder.addLong(1, stIno, 0); }
  public static void addStMode(FlatBufferBuilder builder, long stMode) { builder.addInt(2, (int)stMode, 0); }
  public static void addStNlink(FlatBufferBuilder builder, long stNlink) { builder.addLong(3, stNlink, 0); }
  public static void addStUid(FlatBufferBuilder builder, long stUid) { builder.addInt(4, (int)stUid, 0); }
  public static void addStGid(FlatBufferBuilder builder, long stGid) { builder.addInt(5, (int)stGid, 0); }
  public static void addStRdev(FlatBufferBuilder builder, long stRdev) { builder.addLong(6, stRdev, 0); }
  public static void addStSize(FlatBufferBuilder builder, long stSize) { builder.addLong(7, stSize, 0); }
  public static void addStBlksize(FlatBufferBuilder builder, long stBlksize) { builder.addLong(8, stBlksize, 0); }
  public static void addStBlocks(FlatBufferBuilder builder, long stBlocks) { builder.addLong(9, stBlocks, 0); }
  public static void addStAtime(FlatBufferBuilder builder, long stAtime) { builder.addLong(10, stAtime, 0); }
  public static void addStMtime(FlatBufferBuilder builder, long stMtime) { builder.addLong(11, stMtime, 0); }
  public static void addStCtime(FlatBufferBuilder builder, long stCtime) { builder.addLong(12, stCtime, 0); }
  public static int endStructStat(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

