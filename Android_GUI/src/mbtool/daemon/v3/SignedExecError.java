// automatically generated by the FlatBuffers compiler, do not modify

package mbtool.daemon.v3;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class SignedExecError extends Table {
  public static SignedExecError getRootAsSignedExecError(ByteBuffer _bb) { return getRootAsSignedExecError(_bb, new SignedExecError()); }
  public static SignedExecError getRootAsSignedExecError(ByteBuffer _bb, SignedExecError obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__assign(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public void __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; }
  public SignedExecError __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public String msg() { int o = __offset(4); return o != 0 ? __string(o + bb_pos) : null; }
  public ByteBuffer msgAsByteBuffer() { return __vector_as_bytebuffer(4, 1); }
  public ByteBuffer msgInByteBuffer(ByteBuffer _bb) { return __vector_in_bytebuffer(_bb, 4, 1); }

  public static int createSignedExecError(FlatBufferBuilder builder,
      int msgOffset) {
    builder.startObject(1);
    SignedExecError.addMsg(builder, msgOffset);
    return SignedExecError.endSignedExecError(builder);
  }

  public static void startSignedExecError(FlatBufferBuilder builder) { builder.startObject(1); }
  public static void addMsg(FlatBufferBuilder builder, int msgOffset) { builder.addOffset(0, msgOffset, 0); }
  public static int endSignedExecError(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
}

