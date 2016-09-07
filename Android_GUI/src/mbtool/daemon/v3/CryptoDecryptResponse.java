// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class CryptoDecryptResponse extends Table {
  public static CryptoDecryptResponse getRootAsCryptoDecryptResponse(ByteBuffer _bb) { return getRootAsCryptoDecryptResponse(_bb, new CryptoDecryptResponse()); }
  public static CryptoDecryptResponse getRootAsCryptoDecryptResponse(ByteBuffer _bb, CryptoDecryptResponse obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public CryptoDecryptResponse __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public boolean success() { int o = __offset(4); return o != 0 ? 0!=bb.get(o + bb_pos) : false; }

  public static int createCryptoDecryptResponse(FlatBufferBuilder builder,
      boolean success) {
    builder.startObject(1);
    CryptoDecryptResponse.addSuccess(builder, success);
    return CryptoDecryptResponse.endCryptoDecryptResponse(builder);
  }

  public static void startCryptoDecryptResponse(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addSuccess(FlatBufferBuilder builder, boolean success) { builder.addBoolean(0, success, false); }
  public static int endCryptoDecryptResponse(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

