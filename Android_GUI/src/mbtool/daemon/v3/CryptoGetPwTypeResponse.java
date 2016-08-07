// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class CryptoGetPwTypeResponse extends Table {
  public static CryptoGetPwTypeResponse getRootAsCryptoGetPwTypeResponse(ByteBuffer _bb) { return getRootAsCryptoGetPwTypeResponse(_bb, new CryptoGetPwTypeResponse()); }
  public static CryptoGetPwTypeResponse getRootAsCryptoGetPwTypeResponse(ByteBuffer _bb, CryptoGetPwTypeResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public CryptoGetPwTypeResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public short type() { int o = __offset(4); return o != 0 ? bb.getShort(o + bb_pos) : 0; }

  public static int createCryptoGetPwTypeResponse(FlatBufferBuilder builder,
      short type) {
    builder.startObject(1);
    CryptoGetPwTypeResponse.addType(builder, type);
    return CryptoGetPwTypeResponse.endCryptoGetPwTypeResponse(builder);
  }

  public static void startCryptoGetPwTypeResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addType(FlatBufferBuilder builder, short type) { builder.addShort(0, type, 0); }
  public static int endCryptoGetPwTypeResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

