#include <linux/init.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/string.h>

#define NETLINK_TEST_PROTOCOL 31

static struct sock *nl_sock = NULL;

static void netlink_recv_msg_fn(struct sk_buff *skb_in);

static struct netlink_kernel_cfg cfg = {
        .input = netlink_recv_msg_fn
};

static int __init netlink_greetings_init(void)
{
        nl_sock = netlink_kernel_create(&init_net, NETLINK_TEST_PROTOCOL, &cfg);
        if (!nl_sock) {
                printk(KERN_ERR "Failed to create netlink socket");
                return -ENOMEM;
        }
                
        printk(KERN_INFO "Netlink greetings init ok");
        return 0;
}

static void __exit netlink_greetings_exit(void)
{
        if (nl_sock) {
                netlink_kernel_release(nl_sock);
                nl_sock = NULL;                
        }

        printk(KERN_INFO "Netlink greetings exit ok");
}

static void netlink_recv_msg_fn(struct sk_buff *skb_in) {

}

static void nlmsg_dump(struct nlmsghdr *nlh) {
        printk(KERN_INFO "Netlink message dump: length = %u, type = %hu, flags = %hu, sequence ID = %u, unique (process) ID = %u", 
                nlh->nlmsg_len,
                nlh->nlmsg_type,
                nlh->nlmsg_flags,
                nlh->nlmsg_seq,
                nlh->nlmsg_pid);
}

module_init(netlink_greetings_init);
module_exit(netlink_greetings_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Satti <marcosatti@gmail.com>");
MODULE_DESCRIPTION("Netlink greetings example");
