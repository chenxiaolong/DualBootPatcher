// automatically generated, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class CryptoGetPwTypeRequest extends Table {
  public static CryptoGetPwTypeRequest getRootAsCryptoGetPwTypeRequest(ByteBuffer _bb) { return getRootAsCryptoGetPwTypeRequest(_bb, new CryptoGetPwTypeRequest()); }
  public static CryptoGetPwTypeRequest getRootAsCryptoGetPwTypeRequest(ByteBuffer _bb, CryptoGetPwTypeRequest obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public CryptoGetPwTypeRequest __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }


  public static void startCryptoGetPwTypeRequest(FlatBufferBuilder builder) { builder.startObject(0); }
  public static int endCryptoGetPwTypeRequest(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
};

