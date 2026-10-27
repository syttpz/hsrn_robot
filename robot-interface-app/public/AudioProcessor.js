class AudioProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.buffer = new Float32Array(0); // playback buffer

    this.port.onmessage = (event) => {
      const newSamples = event.data instanceof Float32Array
        ? event.data : new Float32Array(event.data);

      const oldBuffer = this.buffer;
      const combined = new Float32Array(oldBuffer.length + newSamples.length);
      combined.set(oldBuffer);
      combined.set(newSamples, oldBuffer.length);
      this.buffer = combined;
    };
  }

  process(inputs, outputs) {
    const input = inputs[0];
    const output = outputs[0];

    if (input && input[0]) {
      this.port.postMessage(input[0]);
    }

    const samplesNeeded = output[0].length;

    if (this.buffer.length >= samplesNeeded) {
      for (let channel = 0; channel < output.length; channel++) {
        output[channel].set(this.buffer.subarray(0, samplesNeeded));
      }
      this.buffer = this.buffer.subarray(samplesNeeded);
    } else {
      for (let channel = 0; channel < output.length; channel++) {
        output[channel].fill(0); // output silence
      }
    }

    return true;
  }
}

registerProcessor('audio-processor', AudioProcessor);
