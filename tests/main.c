#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "libasm.h"

static int	g_total;
static int	g_fail;

/* Only the first 25 failures are printed, to keep the output readable. */
#define CHECK(cond, ...)                                        \
	do {                                                        \
		g_total++;                                              \
		if (!(cond)) {                                          \
			if (++g_fail <= 25) {                               \
				printf("    [KO] line %d: ", __LINE__);         \
				printf(__VA_ARGS__);                            \
				printf("\n");                                   \
			}                                                   \
		}                                                       \
	} while (0)

static const char	*g_strs[] = {
	"", "a", "ab", "hello", "Hello, World!", "tab\tnl\n", "\xff\xfe\x80",
	"fifteen chars!!", "sixteen chars!!!", "seventeen chars!!",
	"0123456789012345678901234567890123456789",
};
#define NSTR (sizeof(g_strs) / sizeof(*g_strs))

static int	sgn(int x)
{
	return (x > 0) - (x < 0);
}

/*
** Returns a pointer p such that p + len is the first byte of a PROT_NONE page.
** Any read past the end of the buffer (even one byte) segfaults, which is how
** we catch "reads outside the buffer". len must be <= page size.
*/
static unsigned char	*guard_alloc(size_t len, void **base)
{
	long			pg = sysconf(_SC_PAGESIZE);
	unsigned char	*m;

	m = mmap(NULL, pg * 2, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (m == MAP_FAILED)
	{
		perror("mmap");
		exit(1);
	}
	mprotect(m + pg, pg, PROT_NONE);
	*base = m;
	return (m + pg - len);
}

static void	guard_free(void *base)
{
	munmap(base, sysconf(_SC_PAGESIZE) * 2);
}

static int	all_zero(const void *p, size_t n)
{
	const unsigned char	*c = p;

	for (size_t i = 0; i < n; i++)
		if (c[i])
			return (0);
	return (1);
}

/* ------------------------------------------------------------------ */
/*  mandatory                                                          */
/* ------------------------------------------------------------------ */

static void	test_strlen(void)
{
	char	*big;
	size_t	len;
	void	*base;
	char	*p;

	for (size_t i = 0; i < NSTR; i++)
		CHECK(ft_strlen(g_strs[i]) == strlen(g_strs[i]),
			"\"%s\": got %zu, expected %zu",
			g_strs[i], ft_strlen(g_strs[i]), strlen(g_strs[i]));
	big = malloc(100001);
	memset(big, 'x', 100000);
	big[100000] = '\0';
	CHECK(ft_strlen(big) == 100000, "long string: got %zu", ft_strlen(big));
	free(big);
	for (len = 0; len <= 40; len++)
	{
		p = (char *)guard_alloc(len + 1, &base);
		memset(p, 'a', len);
		p[len] = '\0';
		CHECK(ft_strlen(p) == len, "guarded len=%zu: got %zu", len, ft_strlen(p));
		guard_free(base);
	}
}

static void	test_strcpy(void)
{
	char	exp[64];
	char	got[64];
	char	src[64];
	char	*r;
	void	*base;
	char	*p;

	for (size_t i = 0; i < NSTR; i++)
	{
		memset(exp, 0x55, sizeof(exp));
		memset(got, 0x55, sizeof(got));
		strcpy(exp, g_strs[i]);
		r = ft_strcpy(got, g_strs[i]);
		CHECK(r == got, "\"%s\": return value is not dst", g_strs[i]);
		CHECK(memcmp(exp, got, sizeof(got)) == 0,
			"\"%s\": buffer differs (bytes after the NUL must stay untouched)",
			g_strs[i]);
	}
	for (size_t len = 0; len <= 40; len++)
	{
		memset(src, 'z', len);
		src[len] = '\0';
		p = (char *)guard_alloc(len + 1, &base);
		ft_strcpy(p, src);
		CHECK(memcmp(p, src, len + 1) == 0, "guarded len=%zu: wrong copy", len);
		guard_free(base);
	}
}

static void	test_strcmp(void)
{
	static const char	*pairs[][2] = {
		{"", ""}, {"", "a"}, {"a", ""}, {"abc", "abc"}, {"abc", "abd"},
		{"abd", "abc"}, {"abcd", "abc"}, {"abc", "abcd"}, {"\xff", "a"},
		{"a", "\xff"}, {"\x80", "\x7f"}, {"hello", "hello"},
		{"Hello", "hello"}, {"same string", "same string"}, {"az", "ay"},
		{"fifteen chars!!", "fifteen chars!?"},
		{"sixteen chars!!!", "sixteen chars!!!"},
	};

	for (size_t i = 0; i < sizeof(pairs) / sizeof(*pairs); i++)
	{
		int e = sgn(strcmp(pairs[i][0], pairs[i][1]));
		int g = sgn(ft_strcmp(pairs[i][0], pairs[i][1]));
		CHECK(e == g, "(\"%s\", \"%s\"): sign got %d, expected %d",
			pairs[i][0], pairs[i][1], g, e);
	}
}

static void	test_write(void)
{
	int			p[2];
	char		buf[64];
	ssize_t		r;
	const char	*msg = "    (stdout) hello from ft_write\n";

	if (pipe(p) < 0)
	{
		perror("pipe");
		g_fail++;
		return ;
	}
	r = ft_write(p[1], "hello", 5);
	CHECK(r == 5, "returned %zd, expected 5", r);
	r = read(p[0], buf, sizeof(buf));
	CHECK(r == 5 && memcmp(buf, "hello", 5) == 0, "pipe content is wrong");
	r = ft_write(p[1], "hello", 0);
	CHECK(r == 0, "count=0: returned %zd, expected 0", r);
	errno = 0;
	r = ft_write(-1, "x", 1);
	CHECK(r == -1 && errno == EBADF, "bad fd: ret=%zd errno=%d (%s), expected -1/EBADF",
		r, errno, strerror(errno));
	errno = 0;
	r = ft_write(p[1], NULL, 5);
	CHECK(r == -1 && errno == EFAULT, "NULL buf: ret=%zd errno=%d (%s), expected -1/EFAULT",
		r, errno, strerror(errno));
	close(p[0]);
	errno = 0;
	r = ft_write(p[1], "x", 1);
	CHECK(r == -1 && errno == EPIPE, "closed reader: ret=%zd errno=%d (%s), expected -1/EPIPE",
		r, errno, strerror(errno));
	close(p[1]);
	fflush(stdout);
	r = ft_write(1, msg, strlen(msg));
	CHECK(r == (ssize_t)strlen(msg), "stdout: returned %zd", r);
}

static void	test_read(void)
{
	int		p[2];
	int		d;
	char	buf[64];
	ssize_t	r;

	if (pipe(p) < 0)
	{
		perror("pipe");
		g_fail++;
		return ;
	}
	if (write(p[1], "hello world", 11) != 11)
		perror("write");
	r = ft_read(p[0], buf, 5);
	CHECK(r == 5 && memcmp(buf, "hello", 5) == 0, "first read: ret=%zd", r);
	r = ft_read(p[0], buf, sizeof(buf));
	CHECK(r == 6 && memcmp(buf, " world", 6) == 0, "second read: ret=%zd", r);
	r = ft_read(p[0], buf, 0);
	CHECK(r == 0, "count=0: returned %zd, expected 0", r);
	close(p[1]);
	r = ft_read(p[0], buf, sizeof(buf));
	CHECK(r == 0, "EOF: returned %zd, expected 0", r);
	close(p[0]);
	errno = 0;
	r = ft_read(-1, buf, 1);
	CHECK(r == -1 && errno == EBADF, "bad fd: ret=%zd errno=%d (%s), expected -1/EBADF",
		r, errno, strerror(errno));
	d = open(".", O_RDONLY);
	errno = 0;
	r = ft_read(d, buf, 10);
	CHECK(r == -1 && errno == EISDIR, "directory: ret=%zd errno=%d (%s), expected -1/EISDIR",
		r, errno, strerror(errno));
	close(d);
	if (pipe(p) == 0)
	{
		if (write(p[1], "x", 1) != 1)
			perror("write");
		errno = 0;
		r = ft_read(p[0], NULL, 1);
		CHECK(r == -1 && errno == EFAULT, "NULL buf: ret=%zd errno=%d (%s), expected -1/EFAULT",
			r, errno, strerror(errno));
		close(p[0]);
		close(p[1]);
	}
}

static void	test_strdup(void)
{
	char	*d;
	char	src[] = "mutable";

	for (size_t i = 0; i < NSTR; i++)
	{
		d = ft_strdup(g_strs[i]);
		CHECK(d != NULL, "\"%s\": returned NULL", g_strs[i]);
		if (!d)
			continue ;
		CHECK(d != g_strs[i], "\"%s\": returned the same pointer (no copy)", g_strs[i]);
		CHECK(strcmp(d, g_strs[i]) == 0, "\"%s\": content differs", g_strs[i]);
		free(d);
	}
	d = ft_strdup(src);
	src[0] = 'X';
	CHECK(d && strcmp(d, "mutable") == 0, "copy is not independent from source");
	free(d);
}

static void	test_calloc(void)
{
	static const size_t	sizes[] = {1, 7, 8, 16, 100, 1000, 4096, 100000};
	static const size_t	ov[][2] = {
		{SIZE_MAX, 2}, {2, SIZE_MAX}, {SIZE_MAX / 2 + 1, 2},
		{(size_t)1 << 32, (size_t)1 << 32}, {SIZE_MAX, SIZE_MAX},
		{(size_t)1 << 63, 2},
	};
	static const size_t	big[][2] = {{1, SIZE_MAX / 2}, {SIZE_MAX, 1}};
	void				*p;
	unsigned char		*d;

	/* result must be zeroed even when malloc hands back dirty memory */
	for (size_t i = 0; i < sizeof(sizes) / sizeof(*sizes); i++)
	{
		d = malloc(sizes[i]);
		memset(d, 0xAA, sizes[i]);
		free(d);
		p = ft_calloc(sizes[i], 1);
		CHECK(p != NULL && all_zero(p, sizes[i]), "calloc(%zu, 1): not zeroed", sizes[i]);
		free(p);
	}
	p = ft_calloc(10, 8);
	CHECK(p != NULL && all_zero(p, 80), "calloc(10, 8): not zeroed");
	free(p);
	p = ft_calloc(3, 5);
	CHECK(p != NULL && all_zero(p, 15), "calloc(3, 5): not zeroed");
	free(p);
	p = ft_calloc(100000, 1);
	CHECK(p != NULL && all_zero(p, 100000), "calloc(100000, 1): not zeroed");
	free(p);
	/* zero sizes: must not crash (result may be NULL or a unique pointer) */
	free(ft_calloc(0, 0));
	free(ft_calloc(0, 5));
	free(ft_calloc(5, 0));
	/* nmemb * size overflows size_t: must return NULL + ENOMEM */
	for (size_t i = 0; i < sizeof(ov) / sizeof(*ov); i++)
	{
		errno = 0;
		p = ft_calloc(ov[i][0], ov[i][1]);
		CHECK(p == NULL && errno == ENOMEM, "overflow (%zu * %zu): ptr=%p errno=%d (%s)",
			ov[i][0], ov[i][1], p, errno, strerror(errno));
		free(p);
	}
	/* no overflow but too big: malloc fails, errno must be ENOMEM */
	for (size_t i = 0; i < sizeof(big) / sizeof(*big); i++)
	{
		errno = 0;
		p = ft_calloc(big[i][0], big[i][1]);
		CHECK(p == NULL && errno == ENOMEM, "too big (%zu * %zu): ptr=%p errno=%d (%s)",
			big[i][0], big[i][1], p, errno, strerror(errno));
		free(p);
	}
}

/* ------------------------------------------------------------------ */
/*  bonus (make test_bonus)                                            */
/* ------------------------------------------------------------------ */
#ifdef BONUS

#define DEC "0123456789"
#define HEX "0123456789abcdef"

static void	test_atoi_base(void)
{
	static const struct { const char *s, *base; int exp; }	t[] = {
		{"0", DEC, 0}, {"42", DEC, 42}, {"-42", DEC, -42}, {"+42", DEC, 42},
		{"   \t\n\v\f\r42", DEC, 42}, {"  ---+--+1234ab567", DEC, -1234},
		{"2147483647", DEC, INT_MAX}, {"-2147483648", DEC, INT_MIN},
		{"ff", HEX, 255}, {"-ff", HEX, -255}, {"FF", HEX, 0},
		{"101", "01", 5}, {"zz", DEC, 0}, {"12abc", DEC, 12}, {"", DEC, 0},
	};
	static const char	*bad[] = {
		"", "0", "00", "0112", "01+", "0-1", "0 1", "0\t1", "abca",
	};

	for (size_t i = 0; i < sizeof(t) / sizeof(*t); i++)
	{
		int g = ft_atoi_base((char *)t[i].s, (char *)t[i].base);
		CHECK(g == t[i].exp, "(\"%s\", \"%s\"): got %d, expected %d",
			t[i].s, t[i].base, g, t[i].exp);
	}
	for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); i++)
	{
		int g = ft_atoi_base("10", (char *)bad[i]);
		CHECK(g == 0, "invalid base \"%s\": got %d, expected 0", bad[i], g);
	}
}

static int	g_freed;

static void	count_free(void *p)
{
	(void)p;
	g_freed++;
}

static int	cmp_str(void *a, void *b)
{
	return (strcmp(a, b));
}

static t_list	*from_array(const char **v, size_t n)
{
	t_list	*l = NULL;

	while (n--)
		ft_list_push_front(&l, (void *)v[n]);
	return (l);
}

static int	list_matches(t_list *l, const char **v, size_t n)
{
	size_t	i = 0;

	while (l && i < n)
	{
		if (strcmp(l->data, v[i]) != 0)
			return (0);
		l = l->next;
		i++;
	}
	return (l == NULL && i == n);
}

static void	list_free(t_list *l)
{
	t_list	*next;

	while (l)
	{
		next = l->next;
		free(l);
		l = next;
	}
}

static void	test_list_push_size(void)
{
	static const char	*v[] = {"c", "b", "a"};
	t_list				*l = NULL;

	CHECK(ft_list_size(NULL) == 0, "size(NULL) != 0");
	ft_list_push_front(&l, "a");
	CHECK(l && l->next == NULL && strcmp(l->data, "a") == 0, "push on empty list");
	ft_list_push_front(&l, "b");
	ft_list_push_front(&l, "c");
	CHECK(list_matches(l, v, 3), "push_front order wrong (newest must be the head)");
	CHECK(ft_list_size(l) == 3, "size: got %u, expected 3", ft_list_size(l));
	list_free(l);
}

static void	test_list_sort(void)
{
	static const char	*in[] = {"delta", "alpha", "charlie", "bravo", "alpha", "echo"};
	static const char	*out[] = {"alpha", "alpha", "bravo", "charlie", "delta", "echo"};
	static const char	*rev[] = {"e", "d", "c", "b", "a"};
	static const char	*rev_sorted[] = {"a", "b", "c", "d", "e"};
	static const char	*one[] = {"x"};
	t_list				*l;

	l = from_array(in, 6);
	ft_list_sort(&l, cmp_str);
	CHECK(list_matches(l, out, 6), "unsorted input not sorted correctly");
	list_free(l);
	l = from_array(out, 6);
	ft_list_sort(&l, cmp_str);
	CHECK(list_matches(l, out, 6), "already sorted list was changed");
	list_free(l);
	l = from_array(rev, 5);
	ft_list_sort(&l, cmp_str);
	CHECK(list_matches(l, rev_sorted, 5), "reversed list not sorted correctly");
	list_free(l);
	l = from_array(one, 1);
	ft_list_sort(&l, cmp_str);
	CHECK(list_matches(l, one, 1), "single element list changed");
	list_free(l);
	l = NULL;
	ft_list_sort(&l, cmp_str);
	CHECK(l == NULL, "empty list: head must stay NULL");
}

static void	test_list_remove_if(void)
{
	static const char	*v1[] = {"a", "b", "a", "c", "a", "b"};
	static const char	*r1[] = {"b", "c", "b"};
	static const char	*v2[] = {"b", "b", "x", "b"};
	static const char	*r2[] = {"x"};
	static const char	*v3[] = {"z", "z"};
	t_list				*l;

	g_freed = 0;
	l = from_array(v1, 6);
	ft_list_remove_if(&l, "a", cmp_str, count_free);
	CHECK(list_matches(l, r1, 3), "remove \"a\": wrong resulting list");
	CHECK(g_freed == 3, "remove \"a\": free_fct called %d times, expected 3", g_freed);
	list_free(l);
	g_freed = 0;
	l = from_array(v2, 4);
	ft_list_remove_if(&l, "b", cmp_str, count_free);
	CHECK(list_matches(l, r2, 1), "consecutive head removal: wrong resulting list");
	CHECK(g_freed == 3, "free_fct called %d times, expected 3", g_freed);
	list_free(l);
	g_freed = 0;
	l = from_array(v3, 2);
	ft_list_remove_if(&l, "z", cmp_str, count_free);
	CHECK(l == NULL, "remove all: head must be NULL");
	CHECK(g_freed == 2, "free_fct called %d times, expected 2", g_freed);
	g_freed = 0;
	l = from_array(v1, 6);
	ft_list_remove_if(&l, "q", cmp_str, count_free);
	CHECK(list_matches(l, v1, 6) && g_freed == 0, "no match: list must be unchanged");
	list_free(l);
	l = NULL;
	ft_list_remove_if(&l, "a", cmp_str, count_free);
	CHECK(l == NULL, "empty list: head must stay NULL");
}

static void	test_memchr(void)
{
	unsigned char	buf[300];
	static const size_t	ns[] = {0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 33,
		63, 64, 65, 127, 128, 200};
	static const size_t	lens[] = {0, 1, 2, 3, 15, 16, 17, 31, 32, 33, 63, 64,
		65, 100, 1000, 4095, 4096};
	int				cs[5];
	void			*e;
	void			*g;
	void			*base;
	unsigned char	*p;

	srand(42);
	for (int round = 0; round < 200; round++)
	{
		for (size_t i = 0; i < sizeof(buf); i++)
		{
			int r = rand() % 50;
			buf[i] = (r == 0) ? 0 : (r == 1) ? 0xFF : (unsigned char)('a' + rand() % 20);
		}
		cs[0] = 'a' + rand() % 26;
		cs[1] = 0;
		cs[2] = 0xFF;
		cs[3] = 0x100 + 'a';	/* only the low byte counts, like memchr */
		cs[4] = -1;
		for (size_t off = 0; off < 32; off++)
			for (size_t k = 0; k < sizeof(ns) / sizeof(*ns); k++)
				for (int c = 0; c < 5; c++)
				{
					e = memchr(buf + off, cs[c], ns[k]);
					g = ft_simd_memchr(buf + off, cs[c], ns[k]);
					CHECK(e == g, "off=%zu n=%zu c=%d: got %p, expected %p",
						off, ns[k], cs[c], g, e);
				}
	}
	/* the buffer ends exactly where an unmapped page begins */
	for (size_t i = 0; i < sizeof(lens) / sizeof(*lens); i++)
	{
		size_t	len = lens[i];
		long	hits[] = {-1, 0, (long)len / 2, (long)len - 1};

		for (size_t h = 0; h < 4; h++)
		{
			if (hits[h] < 0 && h != 0)
				continue ;
			p = guard_alloc(len, &base);
			memset(p, 'x', len);
			if (hits[h] >= 0)
				p[hits[h]] = 'y';
			g = ft_simd_memchr(p, 'y', len);
			e = (hits[h] >= 0) ? (void *)(p + hits[h]) : NULL;
			CHECK(g == e, "guarded len=%zu hit=%ld: got %p, expected %p", len, hits[h], g, e);
			g = ft_simd_memchr(p, 'z', len);
			CHECK(g == NULL, "guarded len=%zu, absent byte: got %p, expected NULL", len, g);
			guard_free(base);
		}
	}
}

#endif /* BONUS */

/* ------------------------------------------------------------------ */

#define RUN(fn)                                                 \
	do {                                                        \
		int before = g_fail;                                    \
		printf("== %s\n", #fn);                                 \
		fn();                                                   \
		printf("   -> %s\n", g_fail == before ? "OK" : "KO");   \
	} while (0)

int	main(void)
{
	signal(SIGPIPE, SIG_IGN);
	RUN(test_strlen);
	RUN(test_strcpy);
	RUN(test_strcmp);
	RUN(test_write);
	RUN(test_read);
	RUN(test_strdup);
	RUN(test_calloc);
#ifdef BONUS
	RUN(test_atoi_base);
	RUN(test_list_push_size);
	RUN(test_list_sort);
	RUN(test_list_remove_if);
	RUN(test_memchr);
#endif
	printf("\n%d/%d checks passed\n", g_total - g_fail, g_total);
	return (g_fail != 0);
}
