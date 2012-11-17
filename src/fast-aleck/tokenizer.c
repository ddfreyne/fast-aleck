#include <fast-aleck/tokenizer.h>

#include <string.h>

#include <fast-aleck/text-processor.h>

void fa_tokenizer_state_init(fa_tokenizer_state *state) {
	state->fsm_state              = fa_tokenizer_fsm_state_entry;
	state->is_closing_tag         = false;
	state->is_in_excluded_element = false;
	state->is_in_code             = false;
	state->is_in_kbd              = false;
	state->is_in_pre              = false;
	state->is_in_script           = false;
	state->is_in_samp             = false;
	state->is_in_var              = false;
	state->is_in_math             = false;
	state->is_in_textarea         = false;
	state->is_in_title            = false;
}

static void _fa_tokenizer_handle_tag_name(fa_state *state, fa_slice tag_name, fa_token *current_token);

void fa_tokenizer_pass_on_token(fa_state *state, fa_token token) {
	if (0 < token.slice.length) {
		/*
		printf("--- passing on token: ");
		fa_token_print(token, stdout);
		puts("");
		*/

		fa_text_processor_handle_token(state, token);
	}
}

void fa_tokenizer_feed(fa_state *state, char *input, size_t input_length) {
	fa_token current_token = { .slice = { .start = input, .length = 0 }, .type = fa_token_type_text };
	fa_slice tag_name = fa_null_slice;

	while (input_length > 0) {
		char c = *input;

redo:
		switch (state->tokenizer_state.fsm_state) {
			case fa_tokenizer_fsm_state_entry:
				if ('<' == c) {
					state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_start;
					fa_tokenizer_pass_on_token(state, current_token);
					current_token.slice.start  = input;
					current_token.slice.length = 1;
					current_token.type = fa_token_type_inline; // inline unless told otherwise
				} else {
					++current_token.slice.length;
				}
				break;

			case fa_tokenizer_fsm_state_tag_start:
				state->tokenizer_state.is_closing_tag = false;
				switch (c) {
					case '>':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_entry;
						++current_token.slice.length;
						fa_tokenizer_pass_on_token(state, current_token);
						current_token.slice.start  = input+1;
						current_token.slice.length = 0;
						current_token.type = fa_token_type_text;
						break;

					case '/':
						state->tokenizer_state.fsm_state  = fa_tokenizer_fsm_state_tag_name;
						state->tokenizer_state.is_closing_tag = true;
						++current_token.slice.length;
						break;

					case '!':
						// TODO change token type
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_ltexcl;
						++current_token.slice.length;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_name;
						goto redo;
						break;
				}
				break;

			case fa_tokenizer_fsm_state_tag_name:
				++current_token.slice.length;
				if (NULL == tag_name.start) {
					tag_name.start = input;
					tag_name.length = 0;
				}
				{
					switch (c) {
						case ' ':
						case '\t':
							state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_attr;
							_fa_tokenizer_handle_tag_name(state, tag_name, &current_token);
							tag_name = fa_null_slice; // ????
							break;

						case '>':
							state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_entry;
							fa_tokenizer_pass_on_token(state, current_token);
							_fa_tokenizer_handle_tag_name(state, tag_name, &current_token);
							tag_name = fa_null_slice; // ????
							current_token.slice.start  = input+1;
							current_token.slice.length = 0;
							if (state->tokenizer_state.is_in_excluded_element) {
							   current_token.type = fa_token_type_text_raw;
							} else {
							   current_token.type = fa_token_type_text;
							}

							break;

						default:
							++tag_name.length;
							break;
					}
				}
				break;

			case fa_tokenizer_fsm_state_ltexcl:
				++current_token.slice.length;
				switch (c) {
					case '[':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata_start_1;
						break;

					case '-':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_comment_start_1;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_name;
				}
				break;

			case fa_tokenizer_fsm_state_comment_start_1:
				++current_token.slice.length;
				switch (c) {
					case '-':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_comment;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_name;
				}
				break;

			case fa_tokenizer_fsm_state_comment:
				++current_token.slice.length;
				switch (c) {
					case '-':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_comment_end_1;
						break;
				}
				break;

			case fa_tokenizer_fsm_state_comment_end_1:
				++current_token.slice.length;
				switch (c) {
					case '-':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_comment_end_2;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_comment;
				}
				break;

			case fa_tokenizer_fsm_state_comment_end_2:
				++current_token.slice.length;
				switch (c) {
					case '>':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_entry;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_comment;
				}
				break;

			case fa_tokenizer_fsm_state_cdata_start_1:
				++current_token.slice.length;
				switch (c) {
					case 'C':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata_start_2;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_name;
				}
				break;

			case fa_tokenizer_fsm_state_cdata_start_2:
				++current_token.slice.length;
				switch (c) {
					case 'D':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata_start_3;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_name;
				}
				break;

			case fa_tokenizer_fsm_state_cdata_start_3:
				++current_token.slice.length;
				switch (c) {
					case 'A':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata_start_4;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_name;
				}
				break;

			case fa_tokenizer_fsm_state_cdata_start_4:
				++current_token.slice.length;
				switch (c) {
					case 'T':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata_start_5;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_name;
				}
				break;

			case fa_tokenizer_fsm_state_cdata_start_5:
				++current_token.slice.length;
				switch (c) {
					case 'A':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata_start_6;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_name;
				}
				break;

			case fa_tokenizer_fsm_state_cdata_start_6:
				++current_token.slice.length;
				switch (c) {
					case '[':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_tag_name;
				}
				break;

			case fa_tokenizer_fsm_state_cdata:
				++current_token.slice.length;
				switch (c) {
					case ']':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata_end_1;
				}
				break;

			case fa_tokenizer_fsm_state_cdata_end_1:
				++current_token.slice.length;
				switch (c) {
					case ']':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata_end_2;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata;
				}
				break;

			case fa_tokenizer_fsm_state_cdata_end_2:
				++current_token.slice.length;
				switch (c)
				{
					case '>':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_entry;
						break;

					default:
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_cdata;
				}
				break;

			case fa_tokenizer_fsm_state_attr:
				++current_token.slice.length;
				switch (c) {
					case '"':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_attr_dquo;
						break;

					case '\'':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_attr_squo;
						break;

					case '>':
						state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_entry;
						break;
				}
				break;

			case fa_tokenizer_fsm_state_attr_squo:
				++current_token.slice.length;
				if ('\'' == c)
					state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_attr;
				break;

			case fa_tokenizer_fsm_state_attr_dquo:
				++current_token.slice.length;
				if ('"' == c)
					state->tokenizer_state.fsm_state = fa_tokenizer_fsm_state_attr;
				break;
		}

		++input;
		--input_length;
	}

	fa_tokenizer_pass_on_token(state, current_token);
}

#define _FAST_ALECK_SET_FLAGS_FOR_EXCLUDED_ELEMENT(flag) \
	if (state->tokenizer_state.is_closing_tag) \
	{ \
		state->tokenizer_state.is_in_##flag = false; \
		state->tokenizer_state.is_in_excluded_element = \
			state->tokenizer_state.is_in_code || state->tokenizer_state.is_in_kbd || state->tokenizer_state.is_in_pre  || state->tokenizer_state.is_in_script || \
			state->tokenizer_state.is_in_samp || state->tokenizer_state.is_in_var || state->tokenizer_state.is_in_math || state->tokenizer_state.is_in_textarea; \
	} \
	else \
	{ \
		state->tokenizer_state.is_in_##flag = true; \
		state->tokenizer_state.is_in_excluded_element = true; \
	}

static void _fa_tokenizer_handle_tag_name(fa_state *state, fa_slice tag_name, fa_token *current_token)
{
	bool is_block = false;

	if (tag_name.length > 10)
		return;

	switch (tag_name.length)
	{
		case 1: // p
			if ('p' == tag_name.start[0])
				is_block = true;
			break;

		case 2: // br dd dt h1 h2 h3 h4 h5 h6 li
			switch (tag_name.start[0])
			{
				case 'b':
					if ('r' == tag_name.start[1])
						is_block = true;
					break;

				case 'd':
					if ('d' == tag_name.start[1] || 't' == tag_name.start[1])
						is_block = true;
					break;

				case 'h':
					if ('1' <= tag_name.start[1] && '6' >= tag_name.start[1])
						is_block = true;
					break;

				case 'l':
					if ('i' == tag_name.start[1])
						is_block = true;
					break;
			}
			break;

		case 3: // div kbd pre var
			switch (tag_name.start[0])
			{
				case 'd':
					if (0 == strncmp("iv", tag_name.start+1, 2))
					{
						is_block = true;
					}
					break;

				case 'k':
					if (0 == strncmp("bd", tag_name.start+1, 2))
					{
						_FAST_ALECK_SET_FLAGS_FOR_EXCLUDED_ELEMENT(kbd);
					}
					break;

				case 'p':
					if (0 == strncmp("re", tag_name.start+1, 2))
					{
						_FAST_ALECK_SET_FLAGS_FOR_EXCLUDED_ELEMENT(pre);
						is_block = true;
					}
					break;

				case 'v':
					if (0 == strncmp("ar", tag_name.start+1, 2))
					{
						_FAST_ALECK_SET_FLAGS_FOR_EXCLUDED_ELEMENT(var);
					}
					break;
			}
			break;

		case 4: // code math samp
			switch (tag_name.start[0])
			{
				case 'c':
					if (0 == strncmp("ode", tag_name.start+1, 3))
					{
						_FAST_ALECK_SET_FLAGS_FOR_EXCLUDED_ELEMENT(code);
					}
					break;

				case 'm':
					if (0 == strncmp("ath", tag_name.start+1, 3))
					{
						_FAST_ALECK_SET_FLAGS_FOR_EXCLUDED_ELEMENT(math);
					}
					break;

				case 's':
					if (0 == strncmp("amp", tag_name.start+1, 3))
					{
						_FAST_ALECK_SET_FLAGS_FOR_EXCLUDED_ELEMENT(samp);
					}
					break;
			}
			break;

		case 5: // title
			if (0 == strncmp("title", tag_name.start, 5))
			{
				state->tokenizer_state.is_in_title = !state->tokenizer_state.is_closing_tag;
			}
			break;

		case 6: // script
			if (0 == strncmp("script", tag_name.start, 6))
			{
				_FAST_ALECK_SET_FLAGS_FOR_EXCLUDED_ELEMENT(script);
			}
			break;

		case 8: // textarea
			if (0 == strncmp("textarea", tag_name.start, 8))
			{
				_FAST_ALECK_SET_FLAGS_FOR_EXCLUDED_ELEMENT(textarea);
			}
			break;

		case 10: // blockquote
			if (0 == strncmp("blockquote", tag_name.start, 10))
			{
				is_block = true;
			}
			break;
	}

	if (is_block) {
		current_token->type = fa_token_type_block;
	}
}