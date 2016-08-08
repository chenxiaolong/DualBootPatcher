// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class CryptoDecryptRequest extends Table {
  public static CryptoDecryptRequest getRootAsCryptoDecryptRequest(ByteBuffer _bb) { return getRootAsCryptoDecryptRequest(_bb, new CryptoDecryptRequest()); }
  public static CryptoDecryptRequest getRootAsCryptoDecryptRequest(ByteBuffer _bb, CryptoDecryptRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public CryptoDecryptRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public String password() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer passwordAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }

  public static int createCryptoDecryptRequest(FlatBufferBuilder builder,
      int passwordOffset) {
    builder.startObject(1);
    CryptoDecryptRequest.addPassword(builder, passwordOffset);
    return CryptoDecryptRequest.endCryptoDecryptRequest(builder);
  }

  public static void startCryptoDecryptRequest(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addPassword(FlatBufferBuilder builder, int passwordOffset) { builder.addOffset(0, passwordOffset, 0); }
  public static int endCryptoDecryptRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

