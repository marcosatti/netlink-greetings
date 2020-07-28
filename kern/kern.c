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

static void nlmsg_dump(struct nlmsghdr *nlh) {
        printk(KERN_INFO "Netlink message dump: length = %u, type = %hu, flags = %hu, sequence ID = %u, unique (process) ID = %u\n", 
                nlh->nlmsg_len,
                nlh->nlmsg_type,
                nlh->nlmsg_flags,
                nlh->nlmsg_seq,
                nlh->nlmsg_pid);
}

static int __init netlink_greetings_init(void) {
        nl_sock = netlink_kernel_create(&init_net, NETLINK_TEST_PROTOCOL, &cfg);
        if (!nl_sock) {
                printk(KERN_ERR "Failed to create netlink socket\n");
                return -ENOMEM;
        }
                
        printk(KERN_INFO "Netlink greetings init ok\n");
        return 0;
}

static void __exit netlink_greetings_exit(void) {
        if (nl_sock) {
                netlink_kernel_release(nl_sock);
                nl_sock = NULL;                
        }

        printk(KERN_INFO "Netlink greetings exit ok\n");
}

static void netlink_recv_msg_fn(struct sk_buff *skb_in) {
        struct nlmsghdr *nlh_recv, *nlh_reply;
        __u32 pid;
        char *us_data;
        char reply[256];
        struct sk_buff *skb_out;
        int us_data_len, reply_result;

        printk(KERN_INFO "%s() invoked\n", __FUNCTION__);

        nlh_recv = nlmsg_hdr(skb_in);

        nlmsg_dump(nlh_recv);

        pid = nlh_recv->nlmsg_pid;
        us_data = nlmsg_data(nlh_recv);
        us_data_len = nlmsg_len(nlh_recv);

        printk(KERN_INFO "Netlink message from port %u: len = %u, message = %s\n", pid, us_data_len, us_data);

        if (nlh_recv->nlmsg_flags & NLM_F_ACK) {
                memset(reply, 0, sizeof(reply));
                snprintf(reply, sizeof(reply), "Msg from port %u acknowledged\n", pid);

                skb_out = nlmsg_new(sizeof(reply), 0);
                nlh_reply = nlmsg_put(skb_out, 0, nlh_recv->nlmsg_seq, NLMSG_DONE, sizeof(reply), 0);
                strncpy(nlmsg_data(nlh_reply), reply, sizeof(reply));

                reply_result = nlmsg_unicast(nl_sock, skb_out, pid);
                if (reply_result < 0) {
                        printk(KERN_ERR "Error sending data back to user space\n");
                }
        
                kfree_skb(skb_out);
        }
}

module_init(netlink_greetings_init);
module_exit(netlink_greetings_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Satti <marcosatti@gmail.com>");
MODULE_DESCRIPTION("Netlink greetings example");
