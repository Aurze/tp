__kernel void calculerPlaque(
	uint m, float c, float tdh,
	__global float* input,
	__global float* output,
	uint count) {
	uint i = get_global_id(0);
	if (input[i] != 0 && i < count) {
		output[i] = c * input[i] + tdh * (input[i - 1] + input[i + 1] + input[i - m] + input[i + m]);

		barrier(CLK_LOCAL_MEM_FENCE);

		input[i] = output[i];
	}
}
