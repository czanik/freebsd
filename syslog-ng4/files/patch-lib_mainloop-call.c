--- lib/mainloop-call.c.orig	2023-01-10 08:18:59 UTC
+++ lib/mainloop-call.c
@@ -56,12 +56,9 @@ static GMutex main_task_lock;
 static struct iv_list_head main_task_queue = IV_LIST_HEAD_INIT(main_task_queue);
 static struct iv_event main_task_posted;
 
-gpointer
-main_loop_call(MainLoopTaskFunc func, gpointer user_data, gboolean wait)
+static void
+main_loop_wait_for_pending_call_to_finish(void)
 {
-  if (main_loop_is_main_thread())
-    return func(user_data);
-
   g_mutex_lock(&main_task_lock);
 
   /* check if a previous call is being executed */
@@ -79,13 +76,24 @@ main_loop_call(MainLoopTaskFunc func, gpointer user_da
     {
       g_mutex_unlock(&call_info.lock);
     }
+  g_mutex_unlock(&main_task_lock);
+}
 
+gpointer
+main_loop_call(MainLoopTaskFunc func, gpointer user_data, gboolean wait)
+{
+  if (main_loop_is_main_thread())
+    return func(user_data);
+
+  main_loop_wait_for_pending_call_to_finish();
+
   /* call_info.lock is no longer needed, since we're the only ones using call_info now */
   INIT_IV_LIST_HEAD(&call_info.list);
   call_info.pending = TRUE;
   call_info.func = func;
   call_info.user_data = user_data;
   call_info.wait = wait;
+  g_mutex_lock(&main_task_lock);
   iv_list_add(&call_info.list, &main_task_queue);
   iv_event_post(&main_task_posted);
   if (wait)
@@ -134,6 +142,8 @@ main_loop_call_thread_init(void)
 void
 main_loop_call_thread_deinit(void)
 {
+  main_loop_wait_for_pending_call_to_finish();
+
   g_cond_clear(&call_info.cond);
   g_mutex_clear(&call_info.lock);
 }
