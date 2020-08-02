#include <linux/init.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/types.h>
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

        char *us_data = nlmsg_data(nlh);
        printk(KERN_INFO "Netlink message = %s\n", us_data);
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
        printk(KERN_INFO "%s() invoked\n", __FUNCTION__);

        struct nlmsghdr *nlh_recv = nlmsg_hdr(skb_in);

        nlmsg_dump(nlh_recv);

        if (nlh_recv->nlmsg_flags & NLM_F_ACK) {
                printk(KERN_INFO "Netlink message received requesting acknowledgement\n");

                u32 pid = nlh_recv->nlmsg_pid;

                char reply[256];
                memset(reply, 0, sizeof(reply));

                snprintf(reply, sizeof(reply), "Netlink message from port %u acknowledged", pid);

                struct sk_buff *skb_out = nlmsg_new(sizeof(reply), 0);
                struct nlmsghdr *nlh_reply = nlmsg_put(skb_out, 0, nlh_recv->nlmsg_seq, NLMSG_DONE, sizeof(reply), 0);

                char *payload = nlmsg_data(nlh_reply);
                strncpy(payload, reply, sizeof(reply));

                int reply_result = nlmsg_unicast(nl_sock, skb_out, pid);
                if (reply_result < 0) {
                        printk(KERN_ERR "Error sending data back to user space\n");
                }

                /* Do not free nlmsg - nlmsg_unicast does so automatically. */ 

                printk(KERN_INFO "Netlink message acknowledgement done\n");
        }
}

module_init(netlink_greetings_init);
module_exit(netlink_greetings_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Satti <marcosatti@gmail.com>");
MODULE_DESCRIPTION("Netlink greetings example");
